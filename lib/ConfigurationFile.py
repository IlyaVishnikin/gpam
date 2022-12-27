import json

class ConfigurationFile:
	"""
	"""

	def __init__(self, path: str):
		self.path = path
		try:
			with open(self.path, "r") as json_file:
				self.data = json.load(json_file)
		except:
			self.data = {
				"config": {
					"default-vault": "",
					"vaults": []
				}
			}
			self.save()

		self.vaults = self.data["config"]["vaults"]
		self.default_vault = self.data["config"]["default-vault"]

	def save(self) -> None:
		with open(self.path, "w") as json_file:
			json.dump(self.data, json_file, indent=4)
