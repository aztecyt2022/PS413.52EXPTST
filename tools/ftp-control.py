#!/usr/bin/env python3
"""Query and stop DEXCORE's verified FTPS4 server on a trusted local LAN.

Start FTPS4 using the verified DEXCORE host on PS4. This script only controls
already-running FTPS4; it does not enable the legacy HEN FTP server or use
GoldHEN's closed-source software.
"""
import argparse
import ftplib
import ipaddress
import socket
import sys
import time

FTP_PORT = 1337
BANNER = "FTPS4 Server ready."


def safe_local_ip(value):
    try:
        addr = ipaddress.IPv4Address(value)
    except ipaddress.AddressValueError as exc:
        raise argparse.ArgumentTypeError("use your PS4's numeric LAN IPv4 address") from exc
    if not (addr.is_private or addr.is_link_local) or addr.is_multicast or addr.is_unspecified or addr.is_loopback:
        raise argparse.ArgumentTypeError("only an actual PS4 private/link-local LAN IPv4 address is supported")
    return str(addr)


def probe(host, port=FTP_PORT, timeout=3):
    """Return (state, explanation) without assuming a failure means off."""
    ftp = ftplib.FTP()
    ftp.timeout = timeout
    try:
        greeting = ftp.connect(host, port)
        if BANNER not in greeting:
            return "different", "Unexpected server banner: " + repr(greeting)
        return "on", greeting
    except ConnectionRefusedError:
        return "off", "Connection refused; no listener at this address and port."
    except (TimeoutError, socket.timeout):
        return "unknown", "Connection timed out. Check LAN/IP; cannot confirm stopped."
    except (OSError, ftplib.all_errors) as exc:
        return "unknown", type(exc).__name__ + ": " + str(exc)
    finally:
        try:
            ftp.close()
        except OSError:
            pass


def stop(host, port=FTP_PORT, timeout=3, verify_seconds=8):
    """Stop only identifiable FTPS4; verify closure rather than assume success."""
    ftp = ftplib.FTP()
    ftp.timeout = timeout
    try:
        greeting = ftp.connect(host, port)
        if BANNER not in greeting:
            return False, "Refused SHUTDOWN: port belongs to a different FTP server: " + repr(greeting)
        # FTPS4's custom command is independent of HEN's plugin subsystem.
        # No special user account is required by the upstream command handler.
        response = ftp.sendcmd("SHUTDOWN")
        if not response.startswith("200"):
            return False, "FTPS4 did not acknowledge shutdown: " + repr(response)
    except (OSError, ftplib.all_errors) as exc:
        return False, "Could not send a verified shutdown request: " + str(exc)
    finally:
        try:
            ftp.close()
        except OSError:
            pass
    deadline = time.monotonic() + verify_seconds
    while time.monotonic() < deadline:
        state, reason = probe(host, port, timeout=min(timeout, 1))
        if state == "off":
            return True, "FTPS4 acknowledged SHUTDOWN; port no longer accepts connections."
        if state == "different":
            return False, "FTPS4 stopped but another FTP service occupies this port: " + reason
        time.sleep(0.25)
    return False, "FTPS4 acknowledged SHUTDOWN but its listener did not close within the verification window."


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ip", type=safe_local_ip, required=True, help="PS4 private LAN IPv4 address")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--status", action="store_true", help="inspect only; no writes")
    group.add_argument("--stop", action="store_true", help="send FTPS4's own SHUTDOWN command")
    args = parser.parse_args(argv)
    if args.status:
        state, detail = probe(args.ip)
        print("DEXCORE FTPS4:", state.upper(), "(TCP", FTP_PORT, ")")
        print(detail)
        return 0 if state in ("on", "off") else 1
    ok, detail = stop(args.ip)
    print("DEXCORE FTPS4:", "STOPPED" if ok else "NOT VERIFIED")
    print(detail)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())