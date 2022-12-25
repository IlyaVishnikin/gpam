import json

class VaultFile:
	"""
	VaultFile - class that representates vault file
	"""

	def __init__(self, path: str) -> None:
		self.path = path

		try:
			with open(self.path, "r") as f:
				self.data = json.load(f)

		except (json.JSONDecodeError, FileNotFoundError): # recreating configuration file
			vault_file = {
				"vault": {}
			}
			with open(self.path, "w+") as f:
				json.dump(vault_file, f, indent=4)
			with open(self.path, "r") as f:
				self.data = json.load(f)

	def save(self):
		with open(self.path, "w") as f:
			json.dump(self.data, f, indent=4)