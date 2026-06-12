import crypto from "crypto";

const key = Buffer.from(process.env.CRYPTO_KEY, "utf8");
const iv = Buffer.from(process.env.CRYPTO_IV, "utf8");

export function decrypt(ciphertext) {
    const decipher = crypto.createDecipheriv("aes-256-cbc", key, iv);
    let decrypted = decipher.update(ciphertext, "base64", "utf8");
    decrypted += decipher.final("utf8");
    return decrypted;
}
