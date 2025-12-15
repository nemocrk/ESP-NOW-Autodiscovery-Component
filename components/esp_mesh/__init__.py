import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MODE

# NESSUNA dipendenza globale 'wifi'.
# Il Root la richiede esplicitamente.
# Il Nodo usa inizializzazione 'Bare Metal' nel C++.

AUTO_LOAD = ['esp32']

mesh_ns = cg.esphome_ns.namespace('esp_mesh')
EspMesh = mesh_ns.class_('EspMesh', cg.Component)

CONF_MESH_ID = 'mesh_id'
CONF_PMK = 'pmk'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EspMesh),
    cv.Required(CONF_MODE): cv.enum({'ROOT': 0, 'NODE': 1}),
    cv.Required(CONF_MESH_ID): cv.string,
    cv.Required(CONF_PMK): cv.All(cv.string, cv.Length(min=16, max=16)),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_mesh_id(config[CONF_MESH_ID]))
    cg.add(var.set_pmk(config[CONF_PMK]))

    if config[CONF_MODE] == 0: # ROOT
        cg.add_define('IS_ROOT')
        # Il Root DEVE avere WiFi e MQTT configurati in YAML
        if 'mqtt' in cg.get_variable_ids():
            mqtt = await cg.get_variable(cg.get_variable_ids()['mqtt'])
            cg.add(var.set_mqtt(mqtt))
    else: # NODE
        cg.add_define('IS_NODE')
        # Librerie necessarie per l'introspezione automatica
        cg.add_library("esphome/components/sensor", None)
        cg.add_library("esphome/components/switch", None)
        cg.add_library("esphome/components/binary_sensor", None)
