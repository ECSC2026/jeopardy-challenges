# Codescan GitHub infrastructure

To prevent leaking of the challenge solutions via the publicly-visible
forks, we operate our own [GH org](https://github.com/ecsc-2026-codescan).

Via the Terraform in this directory, we instantiate the following within that
org for the 42 country team IDs in the
[competition configuration](https://github.com/ECSC2026/infra/blob/b6fde11953367f2d905f27e66fe46973300e646e/kubernetes/config/ecsc-sb1.yaml)
and the additional team IDs `2517`, `67`, and `79`:

- A "secret" (i.e. invisible-to-others) GH team named
  `codescan-team-<platform team ID>`.
- A private challenge repository named after the lowercase team ID with spaces
  replaced by hyphens that only this GH team has read-only access to.
- Permission for members to fork that repository privately to solve the
  challenge.

The country IDs must be identical to the team UIDs configured in the CTF
platform. The IDs are hard-coded in `main.tf`. To provision them:

```sh
terraform init
terraform apply
```

Competitors register one GitHub username through the deploy-only signup
service. It sends the organization invitation and grants membership in the
matching secret team. They must accept the invitation before cloning.

The signup container uses a separate token in
`GITHUB_SIGNUP_TOKEN_B64_REVERSED`. It must belong to an organization owner
and have organization **Members: write** permission; it needs no repository
permissions. Encode it with `printf %s "$TOKEN" | base64 | tr -d '\n' | rev`.
The service refuses to start without it. This encoding is only obfuscation;
keep the value out of `public/`.

The signup service accepts one valid GitHub username submission per running
container. An atomic directory creation in `/tmp` prevents simultaneous
requests from claiming the same instance. Empty connections, such as probes,
do not consume the attempt. Recreating the challenge instance resets signup.
Anyone with another team's signup instance address could use its attempt, so
the challenge description asks teams to keep it private.

The GitHub provider reads its organization-administrator credential from
`GITHUB_TOKEN`. This can be separate from both runtime tokens.

At runtime, `%TEAMID%` is converted to the same lowercase, hyphenated repository
name. The challenge manifest supplies `GITHUB_TOKEN_B64_REVERSED`, a reversed
base64 encoding of a token that can clone every team repository. Remove the
temporarily added **Members: write** permission from this clone token in
GitHub; only the signup token needs it.
The Git credential helper configured in the Dockerfile decodes it when cloning;
`run.sh` does not handle authentication. This encoding is only obfuscation, not
access control. The value is not included in the `public/` files.
