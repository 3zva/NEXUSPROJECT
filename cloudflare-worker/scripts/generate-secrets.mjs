import { webcrypto, randomBytes } from "node:crypto";
import { writeFile } from "node:fs/promises";

const { subtle } = webcrypto;
const pair = await subtle.generateKey({ name: "ECDSA", namedCurve: "P-256" }, true, ["sign", "verify"]);
const privateJwk = await subtle.exportKey("jwk", pair.privateKey);
const publicJwk = await subtle.exportKey("jwk", pair.publicKey);
const pepper = randomBytes(32).toString("base64url");
const encryption = randomBytes(32).toString("base64");

const local = [
  `LICENSE_HASH_PEPPER=${pepper}`,
  `LICENSE_ENCRYPTION_KEY_B64=${encryption}`,
  `LICENSE_SIGNING_PRIVATE_JWK=${JSON.stringify(privateJwk)}`,
  ""
].join("\n");

await writeFile(".generated-secrets.local.txt", local, { mode: 0o600 });
await writeFile("public-signing-key.json", JSON.stringify(publicJwk, null, 2) + "\n");
console.log("Created .generated-secrets.local.txt and public-signing-key.json");
console.log("Keep the private file secret. Embed only public-signing-key.json in the C++ client and setup wizard.");
