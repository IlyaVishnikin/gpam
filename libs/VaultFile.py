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
				"vault": []
			}
			with open(self.path, "w+") as f:
				json.dump(vault_file, f, indent=4)
			with open(self.path, "r") as f:
				self.data = json.load(f)

	def add_fields(self, **kwargs) -> None:
		if "login" not in kwargs:
			return

		# checks that record with the same login already exists.
		# If record with the same login exists then update fields
		for record in self.data['vault']:
			if record['login'] == kwargs['login'] and record['site'] == kwargs['site'] and record['password'] == kwargs['password']:
				# change already existed fields
				for k, v in kwargs:
					record[k] = v
				return

		self.data['vault'].append(kwargs)

	def save(self):
		with open(self.path, "w") as f:
			json.dump(self.data, f, indent=4)