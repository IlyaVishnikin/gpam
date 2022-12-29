import json

from base64 import b64encode
from Crypto.Random import get_random_bytes
from Crypto.Protocol.KDF import PBKDF2
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad

from argon2 import PasswordHasher
from argon2.exceptions import VerifyMismatchError


class VaultFile:
	"""
	"""

	def __init__(self, path, master_key: str = ""):
		self.path = path
		self.master_key = master_key
		try:
			with open(self.path, "r") as json_file:
				self.data = json.load(json_file)
			PasswordHasher().verify(self.data["master-key"], self.master_key)
		except FileNotFoundError:
			self.data = {
				"salt": b64encode(get_random_bytes(128)).decode(),
				"master-key": "" if not master_key else PasswordHasher().hash(self.master_key),
				"records": []
			}
			self.save()
		except (json.decoder.JSONDecodeError, VerifyMismatchError):
			self.data = {}

	def add_record(self, **fields):
		if not fields:
			raise ValueError("Record fields is not specified")

		if "password" in fields:
			key = PBKDF2(self.master_key, self.data["salt"].encode('ascii'), dkLen=32, count=10**6)
			aes = AES.new(key, AES.MODE_CBC)
			ciphered_password = aes.encrypt(pad(fields["password"].encode('ascii'), AES.block_size))
			aes_iv = aes.iv
			fields["password"] = b64encode(key + aes_iv + ciphered_password).decode()

		record_root = None
		for record in self.data["records"]:
			if record["site"] == fields["site"] and record["login"]:
				record_root = record
		if not record_root:
			self.data["records"].append({})
			record_root = self.data["records"][-1]

		for field_name in fields:
			if field_name not in record_root:
				record_root[field_name] = fields[field_name]

	def save(self):
		with open(self.path, "w") as json_file:
			json.dump(self.data, json_file, indent=4)
