import json

class ConfigurationFile:
	"""
	ConfigurationFile - class that representates configuration file that Gpam using.
	"""
	def __init__(self, path: str) -> None:
		self.path = path

		try:
			with open(self.path, "r") as f:
				self.data = json.load(f)

		except json.JSONDecodeError: # recreating configuration file
			configuration_structure = {
				"configuration": {
					"default-vault": "",
					"vaults": [
					]
				}
			}
			with open(self.path, "w") as f:
				json.dump(configuration_structure, f, indent=4)
			with open(self.path, "r") as f:
				self.data = json.load(f)
		
	def save(self) -> None:
		with open(self.path, "w") as f:
			json.dump(self.data, f, indent=4)

	def add_vault(self, vault_name: str, vault_path) -> None:
		for vault in self.data['configuration']['vaults']:
			if (vault_name in vault['names']):
				return

		self.data['configuration']['vaults'].append({
			"names": [vault_name],
			"master-key": False,
			"vault-path": vault_path
		})
