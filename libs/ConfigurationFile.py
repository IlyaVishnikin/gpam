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

		except (json.JSONDecodeError, FileNotFoundError): # recreating configuration file
			configuration_structure = {
				"configuration": {
					"default-vault": "",
					"vaults": [
					]
				}
			}
			with open(self.path, "w+") as f:
				json.dump(configuration_structure, f, indent=4)
			with open(self.path, "r") as f:
				self.data = json.load(f)
		
	def save(self) -> None:
		with open(self.path, "w") as f:
			json.dump(self.data, f, indent=4)

	def add_vault(self, vault_name: str, vault_path) -> None:
		for vault in self.data['configuration']['vaults']:
			if vault_name in vault['names']:
				return

		self.data['configuration']['vaults'].append({
			"names": [vault_name],
			"master-key": "",
			"path": vault_path
		})

	def add_vault_alias(self, vault_name: str, alias_name: str) -> None:
		for vault in self.data['configuration']['vaults']:
			if vault_name in vault['names'] and alias_name not in vault['names']:
				vault['names'].append(alias_name)

	def set_vault_master_key(self, vault_name: str, master_key: str) -> None:
		for vault in self.data['configuration']['vaults']:
			if vault_name in vault['names']:
				vault['master-key'] = master_key

	def set_default_vault(self, vault_name: str) -> None:
		for vault in self.data['configuration']['vaults']:
			if vault_name in vault['names']:
				self.data['configuration']['default-vault'] = vault_name

	def set_vault_path(self, vault_name: str, new_path: str) -> None:
		for vault in self.data['configuration']['vaults']:
			if vault_name in vault['names']:
				vault['path'] = new_path
