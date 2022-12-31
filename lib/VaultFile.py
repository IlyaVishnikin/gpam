import json
import typing
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

	def update_record(self, site: str, login: str, new: typing.Dict[str, str]) -> None:
		for record in self.data["records"]:
			if record.get("site") != site or record.get("login") != login:
				continue
			for k, v in new.items():
				if k == "password" and self.decrypt_password(record[k]) != v:
					record[k] = self.encrypt_password(v)
					continue
				if record.get(k) == v:
					continue
				record[k] = v

	def delete_record(self, site: str, login: str) -> None:
		if not site or not login:
			return

		for record in self.data["records"]:
			if record.get("site") == site and record.get("login") == login:
				self.data["records"].remove(record)

	def get_all_passwords(self) -> typing.List[typing.Tuple[str, str, str]]:
		all_passwords = []
		for record in self.data["records"]:
			all_passwords.append((
				record.get("site"),
				record.get("login"),
				self.decrypt_password(record.get("password"))
			))
		return all_passwords

	def update_all_sites(self, previos: str, new: str) -> None:
		for record in self.data["records"]:
			if record["site"] == previos:
				record["site"] = new

	def update_master_key(self, master_key):
		if master_key == self.master_key:
			return

		for record in self.data["records"]:
			if "password" not in record:
				continue

			plain_password = self.decrypt_password(record["password"])
			master_key_copy = copy(self.master_key)
			self.master_key = master_key
			record["password"] = self.encrypt_password(plain_password)
			self.master_key = master_key_copy

		self.master_key = master_key
		self.data["master-key"] = "" if not master_key else PasswordHasher().hash(self.master_key)

	def delete_all_sites(self, site):
		for record in self.data["records"]:
			if record.get("site") == site:
				self.data["records"].remove(record)

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

	def save(self):
		with open(self.path, "w") as json_file:
			json.dump(self.data, json_file, indent=4)
