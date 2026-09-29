"""Offline owner-only issuer. The Windows user protects the private key with DPAPI.

Never distribute this tool or its key with the client. Public keys are safe to ship.
"""
import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import re
import time
import uuid
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils

class Blob(ctypes.Structure):
    _fields_ = [('size', wintypes.DWORD), ('data', ctypes.POINTER(ctypes.c_ubyte))]

def protect(data, decrypt=False):
    buffer = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
    source, target = Blob(len(data), buffer), Blob()
    dll = ctypes.WinDLL('crypt32', use_last_error=True)
    fn = dll.CryptUnprotectData if decrypt else dll.CryptProtectData
    fn.argtypes = [ctypes.POINTER(Blob), ctypes.c_void_p, ctypes.c_void_p,
                   ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(Blob)]
    fn.restype = wintypes.BOOL
    if not fn(ctypes.byref(source), None, None, None, None, 1, ctypes.byref(target)):
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        return ctypes.string_at(target.data, target.size)
    finally:
        kernel = ctypes.WinDLL('kernel32')
        kernel.LocalFree.argtypes = [ctypes.c_void_p]
        kernel.LocalFree(target.data)

def load_key(path):
    key = serialization.load_der_private_key(protect(Path(path).read_bytes(), True), password=None)
    if not isinstance(key, ec.EllipticCurvePrivateKey) or not isinstance(key.curve, ec.SECP256R1):
        raise ValueError('Expected ECDSA P-256 private key')
    return key

def public_hex(key):
    numbers = key.public_key().public_numbers()
    return (numbers.x.to_bytes(32, 'big') + numbers.y.to_bytes(32, 'big')).hex()

def sign(key, payload):
    r, s = utils.decode_dss_signature(key.sign(payload, ec.ECDSA(hashes.SHA256())))
    return r.to_bytes(32, 'big') + s.to_bytes(32, 'big')

def issue(key, device, days, now=None):
    if not re.fullmatch('[0-9a-f]{64}', device):
        raise ValueError('Device must contain 64 lowercase hex characters')
    if not 1 <= days <= 3650:
        raise ValueError('Days must be between 1 and 3650')
    now = int(time.time()) if now is None else now
    payload = f'ZB2-LICENSE-1\nMenu-ZB2\n{uuid.uuid4().hex}\n{device}\n{now}\n{now+days*86400}\n'.encode('ascii')
    return 'ZB2L1.' + payload.hex() + '.' + sign(key, payload).hex()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    new = sub.add_parser('keygen')
    new.add_argument('--key', required=True, type=Path)
    new.add_argument('--public', required=True, type=Path)
    emit = sub.add_parser('issue')
    emit.add_argument('--key', required=True, type=Path)
    emit.add_argument('--device', required=True)
    emit.add_argument('--days', required=True, type=int)
    emit.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    if args.command == 'keygen':
        if args.key.exists() or args.public.exists():
            parser.error('Refusing to overwrite existing key material')
        key = ec.generate_private_key(ec.SECP256R1())
        encoded = key.private_bytes(serialization.Encoding.DER, serialization.PrivateFormat.PKCS8,
                                    serialization.NoEncryption())
        args.key.parent.mkdir(parents=True, exist_ok=True)
        with args.key.open('xb') as stream:
            stream.write(protect(encoded))
        args.public.parent.mkdir(parents=True, exist_ok=True)
        args.public.write_text(json.dumps({'algorithm':'ECDSA-P256-SHA256','public_xy':public_hex(key)}, indent=2)+'\n', encoding='ascii')
        print('Created user-bound private key and public verification key. Back up the signing account securely.')
    else:
        token = issue(load_key(args.key), args.device, args.days)
        with args.output.open('x', encoding='ascii') as stream:
            stream.write(token+'\n')
        print('License written:', args.output)

if __name__ == '__main__':
    main()
