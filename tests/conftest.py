# tests/conftest.py
"""
Fixture e configurazione globale per i test del componente esp_mesh.
Fornisce mock per le dipendenze ESP-NOW e ESPHome.
"""

import pytest
from unittest.mock import Mock, MagicMock, patch
import sys
from pathlib import Path

# Aggiungi il path del componente ai percorsi di importazione
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

# ============================================================================
# MOCK GLOBALI ESP-NOW e ESP-IDF
# ============================================================================

class MockEspNow:
    """Mock dell'API ESP-NOW"""
    ESP_OK = 0
    MAX_PEERS = 20
    
    def __init__(self):
        self.peers = {}
        self.packets = []
        self.callbacks = []
        self.pmk = None
    
    def init(self):
        return self.ESP_OK
    
    def set_pmk(self, pmk):
        self.pmk = pmk
        return self.ESP_OK
    
    def add_peer(self, peer_info):
        self.peers[bytes(peer_info['addr'])] = peer_info
        return self.ESP_OK
    
    def del_peer(self, mac):
        if mac in self.peers:
            del self.peers[mac]
        return self.ESP_OK
    
    def is_peer_exist(self, mac):
        return bytes(mac) in self.peers
    
    def send(self, mac, data):
        self.packets.append({'dst': mac, 'data': data})
        return self.ESP_OK
    
    def register_recv_cb(self, callback):
        self.callbacks.append(callback)
        return self.ESP_OK
    
    def receive_packet(self, src_mac, data, rssi=-50):
        """Simula ricezione pacchetto"""
        if self.callbacks:
            mock_info = Mock()
            mock_info.src_addr = src_mac
            mock_info.rx_ctrl = Mock()
            mock_info.rx_ctrl.rssi = rssi
            for cb in self.callbacks:
                cb(mock_info, data, len(data))

class MockWiFi:
    """Mock dell'API WiFi"""
    WIFI_IF_STA = 0
    WIFI_MODE_STA = 2
    WIFI_SECOND_CHAN_NONE = 0
    WIFI_PS_NONE = 0
    
    def __init__(self):
        self.mac = b'\xAA\xBB\xCC\xDD\xEE\xFF'
        self.channel = 1
    
    def get_mac(self, interface):
        return self.mac
    
    def set_channel(self, channel, second_channel):
        self.channel = channel
        return 0
    
    def set_mode(self, mode):
        return 0
    
    def start(self):
        return 0
    
    def init(self, config):
        return 0
    
    def set_ps(self, ps_type):
        return 0

# ============================================================================
# FIXTURES PYTEST
# ============================================================================

@pytest.fixture
def mock_esp_now():
    """Fixture che fornisce un mock di ESP-NOW"""
    return MockEspNow()

@pytest.fixture
def mock_wifi():
    """Fixture che fornisce un mock di WiFi"""
    return MockWiFi()

@pytest.fixture
def mock_mqtt():
    """Fixture per mock MQTT client"""
    mqtt = MagicMock()
    mqtt.publish = MagicMock(return_value=None)
    mqtt.is_connected = MagicMock(return_value=True)
    return mqtt

@pytest.fixture
def mock_app():
    """Fixture per mock ESPHome App (entity registry)"""
    app = MagicMock()
    app.get_sensors = MagicMock(return_value=[])
    app.get_binary_sensors = MagicMock(return_value=[])
    app.get_switches = MagicMock(return_value=[])
    app.get_lights = MagicMock(return_value=[])
    app.get_text_sensors = MagicMock(return_value=[])
    app.get_fans = MagicMock(return_value=[])
    app.get_covers = MagicMock(return_value=[])
    app.get_climates = MagicMock(return_value=[])
    app.get_numbers = MagicMock(return_value=[])
    app.get_selects = MagicMock(return_value=[])
    app.get_locks = MagicMock(return_value=[])
    app.get_valves = MagicMock(return_value=[])
    app.get_alarm_control_panels = MagicMock(return_value=[])
    app.get_events = MagicMock(return_value=[])
    return app

@pytest.fixture
def mock_sensors():
    """Fixture che fornisce sensori mock"""
    sensors = []
    
    # Sensor 1: Temperature
    temp_sensor = MagicMock()
    temp_sensor.get_name = MagicMock(return_value="Temperature")
    temp_sensor.get_object_id_hash = MagicMock(return_value=0x12345678)
    temp_sensor.get_unit_of_measurement_ref = MagicMock(return_value="°C")
    temp_sensor.get_device_class_ref = MagicMock(return_value="temperature")
    sensors.append(temp_sensor)
    
    # Sensor 2: Humidity
    humidity_sensor = MagicMock()
    humidity_sensor.get_name = MagicMock(return_value="Humidity")
    humidity_sensor.get_object_id_hash = MagicMock(return_value=0x87654321)
    humidity_sensor.get_unit_of_measurement_ref = MagicMock(return_value="%")
    humidity_sensor.get_device_class_ref = MagicMock(return_value="humidity")
    sensors.append(humidity_sensor)
    
    return sensors

@pytest.fixture
def mock_binary_sensors():
    """Fixture per sensori binari mock"""
    sensors = []
    
    # Binary Sensor 1: Motion
    motion = MagicMock()
    motion.get_name = MagicMock(return_value="Motion")
    motion.get_object_id_hash = MagicMock(return_value=0x11111111)
    motion.get_device_class_ref = MagicMock(return_value="motion")
    sensors.append(motion)
    
    # Binary Sensor 2: Door
    door = MagicMock()
    door.get_name = MagicMock(return_value="Door")
    door.get_object_id_hash = MagicMock(return_value=0x22222222)
    door.get_device_class_ref = MagicMock(return_value="door")
    sensors.append(door)
    
    return sensors

@pytest.fixture
def node_config():
    """Configurazione di NODE valida"""
    return {
        'mode': 1,  # NODE
        'mesh_id': 'SmartHome_Mesh',
        'pmk': '1234567890ABCDEF'
    }

@pytest.fixture
def root_config():
    """Configurazione di ROOT valida"""
    return {
        'mode': 0,  # ROOT
        'mesh_id': 'SmartHome_Mesh',
        'pmk': '1234567890ABCDEF'
    }

# ============================================================================
# MARKER PYTEST
# ============================================================================

def pytest_configure(config):
    """Registra marker custom"""
    config.addinivalue_line("markers", "slow: mark test as slow (deselect with '-m \"not slow\"')")
    config.addinivalue_line("markers", "integration: mark test as integration test")
    config.addinivalue_line("markers", "unit: mark test as unit test")
    config.addinivalue_line("markers", "performance: mark test as performance test")
    config.addinivalue_line("markers", "node: mark test as node-specific")
    config.addinivalue_line("markers", "root: mark test as root-specific")
