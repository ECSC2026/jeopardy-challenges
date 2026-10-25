
## Trust Issues
The challenges implements a TLS client uses Mozilla's NSS library. The challenge uses a custom certificate verification callback. The callback validates the server certificate using `CERT_PKIXVerifyCert`. This method returns a status code that describes whether certificate validation succeeded. It may also return an error log if requested by the caller.

The vulnerability is that the challenge does not validate the return code of `CERT_PKIXVerifyCert`, but only whether the error log is empty. In some cases, the NSS library returns an empty error log, even if a validation error occurs. This behavior can be triggered by providing a malformed email address record in the certificate for example.
