//
//  options.cpp
//  macho-inject
//
//  Created by Jon Gabilondo on 20/03/2017.
//

#include "cmd_options.hpp"

int cmd_opt_add_descriptions(boost::program_options::options_description & description)
{
    description.add_options()
    ("help,h", "Print help message.")
    ("inject-framework,f", boost::program_options::value< std::string >(), "Inject a framework into the App. arg: the full path the the framework.")
    ("sign,s", boost::program_options::value< std::string >(), "Sig the app with a certificate. arg: the name of the certificate as shown in the keychain or '-' for adhoc.")
    ("provision,p", boost::program_options::value< std::string >(), "Attach a mobile provision profile to the App. arg: full path to the .mobileprovision file.")
    ("entitlements,e", boost::program_options::value< std::string >(), "Add entitlements file. arg: full path to the entitlements file.")
    ("resource-rules,r", boost::program_options::value< std::string >(), "Your custom resource rules file. arg: full path to resource file.")
    ("output-dir,o", boost::program_options::value< std::string >(), "An optional path of the folder to place the new ipa. The folder must exist.")
    ("output-name,n", boost::program_options::value< std::string >(), "An optional name for the new app/ipa.")
    ("original-res-rules", "If resource rules file it exists use the original resources rules file, the one inside the the original ipa: ResourceRules.plist.")
    ("generic-res-rules", "Apply a generic Resouces Rules definition. In case the codesign fails due to problems with the resources try this option. A generic template will be used.")
    ("force-res-rules", "It will try as if parameters --original-res-rules was set, if no original rules found it will be as if parameter --generic-res-rules defined.")
    ("remove-entitlements", "Remove entitlements file from App.")
    ("remove-signature", "Remove code signature from binary and '_CodeSignature' folder.")
    ("input-file,i", boost::program_options::value< std::string >(), "Full path to the file App/IPA file to operate on.")
    ("hardened,H", "Harden the binary (equivalent to option 'runtime'")
    ("verbose,v", "Verbose.")
    ("version,V", "Print version.")
    ;
    return 0;
}

int cmd_opt_parse_options(int argc,
                  const char * argv[],
                  boost::program_options::options_description & description,
                  boost::program_options::variables_map &vm)
{
    try {
        boost::program_options::positional_options_description pos_desc;
        pos_desc.add("input-file", -1);
        
        boost::program_options::store(
                                      boost::program_options::command_line_parser(argc, argv).
                                      options(description).positional(pos_desc).run(),
                                      vm
                                      );
        //boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
    } catch (boost::program_options::error &error) {
        std::cout << error.what() << "\n";
        return ERR_General_Error;
    }
    return 0;
}


