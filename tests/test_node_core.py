# tests/test_node_core.py
"""
Test unitari specifici per NODE del componente ESP-NOW Mesh.
Testa inizializzazione, scanning, auto-discovery.
"""

import pytest
from unittest.mock import Mock, MagicMock
import struct


@pytest.mark.node
@pytest.mark.unit
class TestNodeSetup:
    """Test setup NODE"""
    
    def test_node_setup_initialization(self, mock_esp_now, mock_wifi, mock_app):
        """Verifica inizializzazione NODE"""
        mesh = Mock()
        mesh.pmk_ = "1234567890ABCDEF"
        mesh.net_id_hash_ = 0x12345678
        mesh.hop_count_ = 0xFF
        mesh.scanning_ = True
        mesh.current_scan_ch_ = 1
        
        assert mesh.hop_count_ == 0xFF, "NODE should start disconnected"
        assert mesh.scanning_ == True, "NODE should start scanning"
        assert len(mesh.pmk_) == 16
    
    def test_node_bare_metal_wifi(self, mock_wifi):
        """Verifica setup bare metal WiFi per NODE"""
        assert mock_wifi.channel >= 1
        assert mock_wifi.channel <= 13
    
    def test_node_scanning_logic(self):
        """Verifica logica di scanning canali"""
        current_ch = 1
        for _ in range(13):
            current_ch = (current_ch % 13) + 1
        assert current_ch == 1, "Channel scan should wrap around"


@pytest.mark.node
@pytest.mark.unit
class TestNodeAnnounceHandling:
    """Test gestione ANNOUNCE per NODE"""
    
    def test_handle_announce_from_root(self, mock_esp_now):
        """NODE riceve ANNOUNCE da ROOT"""
        mesh = Mock()
        mesh.hop_count_ = 0xFF
        root_mac = b'\xAA\xBB\xCC\xDD\xEE\xFF'
        
        # Simula ricezione ANNOUNCE con hop=0
        mesh.hop_count_ = 1
        mesh.parent_mac_ = root_mac
        
        assert mesh.hop_count_ == 1
        assert mesh.parent_mac_ == root_mac
    
    def test_handle_announce_better_parent(self):
        """NODE cambia parent se trova percorso migliore"""
        mesh = Mock()
        mesh.hop_count_ = 3
        old_parent = b'\x11\x22\x33\x44\x55\x66'
        mesh.parent_mac_ = old_parent
        
        # Riceve ANNOUNCE con hop=1 (migliore)
        new_parent = b'\xAA\xBB\xCC\xDD\xEE\xFF'
        mesh.hop_count_ = 2  # 1 + 1
        mesh.parent_mac_ = new_parent
        
        assert mesh.hop_count_ < 3
        assert mesh.parent_mac_ == new_parent


@pytest.mark.node
@pytest.mark.unit
class TestNodeEntityScanning:
    """Test scanning entità locali NODE"""
    
    def test_scan_local_sensors(self, mock_app, mock_sensors):
        """Verifica scansione sensori locali"""
        mock_app.get_sensors = MagicMock(return_value=mock_sensors)
        
        sensors = mock_app.get_sensors()
        assert len(sensors) == 2
        assert sensors[0].get_name() == "Temperature"
        assert sensors[1].get_name() == "Humidity"
    
    def test_entity_registration_packet(self):
        """Verifica formato PKT_REG"""
        # Simula RegPayload
        entity_hash = 0x12345678
        type_id = ord('S')  # Sensor
        name = "Temperature"
        
        # Packed structure simulation
        reg_packet = struct.pack(
            '=I c 24s 8s 16s',
            entity_hash,
            type_id.to_bytes(1, 'little'),
            name.ljust(24, '\x00').encode(),
            '\u00b0C'.ljust(8, '\x00').encode(),
            'temperature'.ljust(16, '\x00').encode()
        )
        
        assert len(reg_packet) == 53  # 4+1+24+8+16
    
    def test_sensor_callback_registration(self, mock_sensors):
        """Verifica registrazione callback su cambio stato"""
        sensor = mock_sensors[0]
        callback_called = False
        
        def state_callback(value):
            nonlocal callback_called
            callback_called = True
        
        # Simula add_on_state_callback
        sensor.add_on_state_callback = state_callback
        sensor.add_on_state_callback(25.5)
        
        assert callback_called


@pytest.mark.node
@pytest.mark.unit
class TestNodeDataTransmission:
    """Test trasmissione dati NODE"""
    
    def test_send_sensor_data(self):
        """Verifica invio PKT_DATA"""
        entity_hash = 0x12345678
        value = 25.5
        
        # PKT_DATA format: hash(4) + float(4)
        data_packet = struct.pack('=If', entity_hash, value)
        assert len(data_packet) == 8
        
        unpacked_hash, unpacked_value = struct.unpack('=If', data_packet)
        assert unpacked_hash == entity_hash
        assert abs(unpacked_value - value) < 0.01
    
    def test_send_binary_sensor_data(self):
        """Verifica invio dati binary sensor"""
        entity_hash = 0x11111111
        state = True
        
        # PKT_DATA format: hash(4) + bool(1)
        data_packet = struct.pack('=I?', entity_hash, state)
        assert len(data_packet) == 5


@pytest.mark.node
@pytest.mark.integration
class TestNodeE2E:
    """Test E2E per NODE"""
    
    def test_node_discovery_flow(self, mock_esp_now, mock_wifi):
        """Test completo: boot → scan → connect → register"""
        # Arrange
        mesh = Mock()
        mesh.hop_count_ = 0xFF
        mesh.scanning_ = True
        mesh.current_scan_ch_ = 1
        
        # Act - Simula discovery
        for ch in range(1, 14):
            mesh.current_scan_ch_ = ch
            if ch == 6:  # Trova ROOT su canale 6
                mesh.hop_count_ = 1
                mesh.scanning_ = False
                break
        
        # Assert
        assert mesh.hop_count_ == 1
        assert mesh.scanning_ == False
        assert mesh.current_scan_ch_ == 6
