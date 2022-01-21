# macho-inject
OS X command line tool to inject Frameworks and dylibs on mach-o binaries. It does the injection of the framework and the codesigning. It can be used to codesign Apps only as well.

## Build
Open the macho-inject.xcodeproj and build it. The result is a command line tool called "macho-inject".

## Usage

$ macho-inject --inject_framework framework --codesign certificate -p provision [-v] [-h] [-V]  ipa_file  

Allowed options:

  -h [ --help ]                 Print help message.\
  -f [ --inject-framework ] arg Inject a framework into the IPA. arg: the full path the the framework.\
  -c [ --certificate ] arg      Codesign the app with a certificate. arg: the name of the certificate as shown in the keychain.\
  -p [ --provision ] arg        Attach a mobile provision profile to the App. arg: full path to the .mobileprovision file.\
  -e [ --entitlements ] arg     Add entitlements file. arg: full path to the entitlements file.\
  -r [ --resource-rules ] arg   Your custom resource rules file. arg: full path to resource file.\
  -d [ --output-dir ] arg       An optional path of the folder to place the new ipa. The folder must exist.\
  -n [ --output-name ] arg      An optional name for the new ipa.\
  --original-res-rules          If resource rules file it exists use the original resources rules file, the one inside the the original ipa: ResourceRules.plist.\
  --generic-res-rules           Apply a generic Resouces Rules definition. In case the codesign fails due to problems with the resources try this option. A generic template will be used.\
  --force-res-rules             It will try as if parameters --original-res-rules was set, if no original rules found it will be as if parameter --generic-res-rules defined.\
  --remove-entitlements         Remove entitlements file from IPA.\
  --remove-code-signature       Remove code signature folder '_CodeSignature' from IPA.\
  -v [ --verbose ]              Verbose.\
  -V [ --version ]              Print version.\
  -i [ --input-file ] arg       IPA file to operate on. arg: full path to the IPA file.

