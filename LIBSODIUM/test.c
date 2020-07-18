#include <sodium.h>
    #define PASSWORD "Correct Horse Battery Staple"
#define KEY_LEN crypto_secretbox_KEYBYTES

int main(void)
{
    if (sodium_init() < 0) {
        /* panic! the library couldn't be initialized, it is not safe to use */
    }
    else
    {
    printf("worked\n");


unsigned char salt[crypto_pwhash_SALTBYTES];
unsigned char key[KEY_LEN];

/* Choose a random salt */
randombytes_buf(salt, sizeof salt);

if (crypto_pwhash
    (key, sizeof key, PASSWORD, strlen(PASSWORD), salt,
     crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
     crypto_pwhash_ALG_DEFAULT) != 0) {
    /* out of memory */
}
    }
    return 0;
}
