<?php


$key = "ratso";

$string = "hello";

$crypted =  mcrypt_generic($key, $string);

echo "raw: ".$string."\n";
echo "crypted: ".$crypted."\n";
//echo "uncrypted: ".mcrypt_decrypt(MCRYPT_CRYPT, $key, $crypted, MCRYPT_MODE_CBC)."\n";



/*


$key = "this is a secret key";
$input = "Let us meet at 9 o'clock at the secret place.";

$encrypted_data = mcrypt_ecb (MCRYPT_3DES, $key, $input, MCRYPT_ENCRYPT);

*/

?>