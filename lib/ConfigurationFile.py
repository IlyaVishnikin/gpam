import json

class ConfigurationFile:
	"""
	"""

	def __init__(self, path: str):
		self.path = path
		try:
			with open(self.path, "r") as json_file:
				self.data = json.load(json_file)
		except FileNotFoundError:
			self.data = {
				"config": {
					"default-vault": "",
					"vaults": []
				}
			}
			self.save()
		except json.decoder.JSONDecodeError:
			self.data = {}

		self.vaults = self.data["config"]["vaults"] if self.data else {}
		self.default_vault = self.data["config"]["default-vault"] if self.data else {}

	def add_vault(self, vault_name: str, path: str, master_key: str = "") -> None:
		if not vault_name:
			raise ValueError("The vault name shouldn't be empty")

		for vault in self.vaults:
			if vault_name in vault["names"]:
				return

		self.vaults.append({
			"names": [vault_name],
			"path": path,
		})
		self.default_vault = self.data["config"]["default-vault"] = vault_name

	def get_all_vault_names(self) -> [str]:
		vault_names = []
		for vault in self.vaults:
			vault_names.append(*vault["names"])
		return vault_names

	def save(self) -> None:
		with open(self.path, "w") as json_file:
			json.dump(self.data, json_file, indent=4)
