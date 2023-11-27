from pathlib import Path
from hashlib import sha256

from nacl.signing import VerifyKey
from nacl.exceptions import BadSignatureError


ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()


# Check if a signature of a given message is valid
def check_signature_validity(public_key: bytes, signature: bytes, message: bytes) -> bool:
    pk = VerifyKey(public_key[1:])

    return pk.verify(signature=signature, smessage=message)

