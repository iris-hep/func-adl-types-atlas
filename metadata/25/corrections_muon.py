muon_container = '{{calib.muon_collection}}_{{calib.muon_working_point}}_{{calib.muon_isolation}}_Calib'

config.addBlock ('Muons')
config.setOptions (containerName=muon_container)
config.setOptions (recalibratePhyslite=False)
config.setOptions (decorateTruth=True)

config.addBlock ('Muons.WorkingPoint')
config.setOptions (containerName=muon_container)
config.setOptions (selectionName='MuonSelection')
config.setOptions (quality='{{calib.muon_working_point}}')
config.setOptions (isolation='{{calib.muon_isolation}}')

config.addBlock ('Thinning')
config.setOptions (containerName=muon_container)
config.setOptions (selectionName='MuonSelection')
config.setOptions (outputName='OutMuons')

# Output muon_collection = OutMuons_{{ sys_error }}