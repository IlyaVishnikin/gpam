import json

from base64 import b64encode
from Crypto.Random import get_random_bytes
from argon2 import PasswordHasher


class VaultFile:
	"""
	"""

	def __init__(self, path, master_key: str = ""):
		self.path = path
		self.master_key = master_key
		try:
			with open(self.path, "r") as json_file:
				self.data = json.load(json_file)
		except FileNotFoundError:
			self.data = {
				"salt": b64encode(get_random_bytes(128)).decode(),
				"master-key": "" if not master_key else PasswordHasher().hash(master_key),
				"records": []
			}
			self.save()
		except json.decoder.JSONDecodeError:
			self.data = {}

	def save(self):
		with open(self.path, "w") as json_file:
			json.dump(self.data, json_file, indent=4)
