# NEXUS Web Setup Wizard requirements

Codex must create a native x64 Windows setup wizard, `NEXUS-Web-Setup.exe`.

## User experience

- Match NEXUS dark navy and purple palette.
- Use the supplied NEXUS icon.
- No console windows.
- Pages: Welcome, legal acceptance, destination, confirmation, download progress, install progress, finish.
- Default per-user install: `%LOCALAPPDATA%\HighCloudNEXUS\NEXUS`.
- Optional desktop shortcut.
- Register a normal uninstaller.

## Secure delivery

1. Fetch the Worker’s signed release manifest.
2. Verify ECDSA P-256 signature with the embedded public key.
3. Validate HTTPS URL, version, size limits, and minimum Windows build.
4. Download with WinHTTP to a random file in `%TEMP%`.
5. Verify exact SHA-256.
6. Extract without invoking PowerShell, cmd.exe, batch files, or downloaded scripts.
7. Stage to a new version directory and atomically switch only after verification.
8. Preserve the previous version for rollback.
9. Delete all temporary data on cancellation or failure.

## Dependency strategy

Compile with `/MT` when compatible, and include every runtime DLL in the release ZIP. The setup wizard should not fetch random runtime libraries. If a prerequisite cannot be statically linked, bundle its official signed installer or retrieve it only from the vendor's official stable URL and verify Authenticode.

## Reverse engineering

The setup wizard is public and must contain no private secret. Its protection comes from verifying a signed manifest, not hiding URLs or algorithms. Build Release x64, use standard exploit mitigations, remove PDBs from distribution, and Authenticode-sign when possible. Do not add invasive anti-debugging or AV-bypass behavior.
