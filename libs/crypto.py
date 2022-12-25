from base64 import b64encode, b64decode

from Crypto.Random import get_random_bytes
from Crypto.Protocol.KDF import PBKDF2

from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad


def aes_encrypt(password: str, salt: str) -> str:
	key = PBKDF2(password, b"{salt_bytes}", dkLen=32, count=10**6)
	cipher = AES.new(key, AES.MODE_CBC)
	ciphered_data = cipher.encrypt(pad(b"{password}", AES.block_size))
	return b64encode(ciphered_data).decode()
