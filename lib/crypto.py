import string
import random

def generate_password(length: int) -> str:
	if length < 0:
		raise ValueError(f"Invalid length: {length}")

	alphabet = string.ascii_letters + string.digits + string.punctuation
	return ''.join(random.choices(alphabet, k=length))
