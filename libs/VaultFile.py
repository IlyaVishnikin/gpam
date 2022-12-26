import json

from base64 import b64encode, b64decode
from argon2 import PasswordHasher

from Crypto.Random import get_random_bytes
from Crypto.Protocol.KDF import PBKDF2

from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad

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
				"master-key": "",
				"vault": []
			}
			with open(self.path, "w+") as f:
				json.dump(vault_file, f, indent=4)
			with open(self.path, "r") as f:
				self.data = json.load(f)

	def add_fields(self, master_key: str, **kwargs) -> None:
		if "login" not in kwargs or "site" not in kwargs:
			return

		if "password" in kwargs and self.data['master-key']:
			PasswordHasher().verify(self.data["master-key"], master_key)

			# encrypt password with master key
			key = PBKDF2(master_key, b"{self.data['master-key']}", dkLen=32, count=10**6)
			cipher = AES.new(key, AES.MODE_CBC)
			ciphered_data = cipher.encrypt(pad(b"{kwargs['password']}", AES.block_size))
			kwargs["password"] = b64encode(ciphered_data).decode()
			kwargs["aes_iv"] = str(cipher.iv)

		for record in self.data['vault']:
			if record['login'] == kwargs['login'] and record['site'] == kwargs['site']:
				for k, v in kwargs:
					if k not in record:
						record[k] = v
				return

		self.data['vault'].append(kwargs)

	def set_master_key(self, master_key: str) -> None:
		if not master_key:
			return

		self.data['master-key'] = PasswordHasher().hash(master_key)

	def get_master_key(self) -> str:
		return self.data['master-key']

	def save(self):
		with open(self.path, "w") as f:
			json.dump(self.data, f, indent=4)

