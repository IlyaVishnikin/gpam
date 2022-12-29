import json

class ConfigurationFile:
	"""
	"""

	def __init__(self, path: str):
		self.path = path
		try:
			self.read()
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

	def get_vault_path(self, name) -> str:
		if not name:
			return ""

		for vault in self.vaults:
			if name in vault["names"]:
				return vault["path"]

		return ""

	def read(self):
		self.data = {}
		with open(self.path, "r") as json_file:
				self.data = json.load(json_file)

	def update_vault_name(self, previos_name: str, new_name: str) -> None:
		if not previos_name or not new_name:
			return

		for vault in self.vaults:
			if previos_name in vault["names"]:
				vault["names"].remove(previos_name)
				vault["names"].append(new_name)
				return

		raise KeyError(f"Vault with the name {previos_name} is not exists")

	def save(self) -> None:
		with open(self.path, "w") as json_file:
			json.dump(self.data, json_file, indent=4)
