import json
from copy import copy

from base64 import b64encode, b64decode
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
			fields["password"] = self.encrypt_password(fields["password"])

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

	def encrypt_password(self, password) -> str:
		key = PBKDF2(self.master_key, self.data["salt"].encode('ascii'), dkLen=32, count=10**6)
		aes = AES.new(key, AES.MODE_CBC)
		ciphered_password = aes.encrypt(pad(password.encode('ascii'), AES.block_size))
		aes_iv = aes.iv
		return b64encode(aes_iv + ciphered_password).decode()

	def decrypt_password(self, password) -> str:
		password_bytes = b64decode(password)
		key = PBKDF2(self.master_key, self.data["salt"].encode("ascii"), dkLen=32, count=10**6)
		aes_iv, ciphered_password = password_bytes[:16], password_bytes[16:]
		cipher = AES.new(key, AES.MODE_CBC, iv=aes_iv)
		plain_password = unpad(cipher.decrypt(ciphered_password), AES.block_size)
		return plain_password.decode("utf-8")

	def update_master_key(self, master_key):
		for record in self.data["records"]:
			if "password" in record:
				decrypted_password = self.decrypt_password(record["password"])
				master_key_copy = copy(self.master_key)
				self.master_key = master_key
				record["password"] = self.encrypt_password(decrypted_password)
				self.master_key = master_key_copy

		self.master_key = master_key
		self.data["master-key"] = "" if not master_key else PasswordHasher().hash(self.master_key)

	def save(self):
		with open(self.path, "w") as json_file:
			json.dump(self.data, json_file, indent=4)
