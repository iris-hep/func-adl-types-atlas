electron_container_name = '{{calib.electron_collection}}_{{calib.electron_working_point}}_{{calib.electron_isolation}}_Calib'

config.addBlock ('Electrons')
config.setOptions (containerName=electron_container_name)
config.setOptions (recalibratePhyslite=False)

config.addBlock ('Electrons.WorkingPoint')
config.setOptions (containerName=electron_container_name)
config.setOptions (selectionName='ElectronSelection')
config.setOptions (forceFullSimConfig=True)
config.setOptions (identificationWP='{{calib.electron_working_point}}')
config.setOptions (isolationWP='{{calib.electron_isolation}}')

config.addBlock ('Thinning')
config.setOptions (containerName=electron_container_name)
config.setOptions (selectionName='ElectronSelection')
config.setOptions (outputName='OutElectrons')

# Output electron_collection = OutElectrons_{{ sys_error }}