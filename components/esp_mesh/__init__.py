import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MODE, CONF_CHANNEL

# --- BEST PRACTICES: COSTANTI E NAMESPACE ---
CONF_MESH_ID = 'mesh_id'
CONF_PMK = 'pmk'

# Definiamo il namespace C++
mesh_ns = cg.esphome_ns.namespace('esp_mesh')
EspMesh = mesh_ns.class_('EspMesh', cg.Component)

# --- AUTO LOADING ---
# Carica automaticamente i componenti interni necessari.
# Questo evita l'errore VCSBaseException e rende disponibili gli header C++
AUTO_LOAD = ['esp32', 'sensor', 'switch', 'binary_sensor', 'text_sensor']

# --- CONFIGURATION VALIDATION ---
# Usiamo cv.All per combinare lo Schema a dizionario con il controllo di piattaforma
CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(EspMesh),
        cv.Required(CONF_MODE): cv.enum({'ROOT': 0, 'NODE': 1}),
        cv.Required(CONF_MESH_ID): cv.string,
        cv.Required(CONF_PMK): cv.All(cv.string, cv.Length(min=16, max=16)),
    }).extend(cv.COMPONENT_SCHEMA),
    
    # Questo validatore va messo FUORI dal dizionario, dentro cv.All
    cv.only_on(['esp32'])
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_mesh_id(config[CONF_MESH_ID]))
    cg.add(var.set_pmk(config[CONF_PMK]))

    # --- LOGICA DI GENERAZIONE CODICE ---
    if config[CONF_MODE] == 0: # ROOT
        cg.add_define('IS_ROOT')
        
        # VALIDAZIONE CROSS-COMPONENT (Safe Check)
        if 'mqtt' not in cg.get_variable_ids():
            raise cv.Invalid("Il Nodo ROOT richiede la presenza del componente 'mqtt:' nella configurazione.")
        if 'wifi' not in cg.get_variable_ids():
            raise cv.Invalid("Il Nodo ROOT richiede la presenza del componente 'wifi:' per connettersi al Broker.")

        mqtt = await cg.get_variable(cg.get_variable_ids()['mqtt'])
        cg.add(var.set_mqtt(mqtt))
        
    else: # NODE
        cg.add_define('IS_NODE')
        # Se nel YAML del nodo c'è un canale fisso (opzionale), lo passiamo
        if CONF_CHANNEL in config:
             cg.add(var.set_channel(config[CONF_CHANNEL]))