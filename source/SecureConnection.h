#include "IClientServerTCP.h"
#include "SecureMessageCreator.h"
#include "CertificationValidator.h"
#include <exception>
#include <fstream>

#define BUFF_SIZE 4096

class SecureConnectionException : public std::exception
{
    public:
    virtual const char *what() const throw() = 0;
};

class HashNotValidException : public SecureConnectionException
{
    public:
    const char *what() const throw()
    {
        return "Not valid hash during checking";
    }
};

class ErrorOnOtherPartException : public SecureConnectionException
{
    public:
    const char *what() const throw()
    {
        return "Error on other part exception";
    }
};

class FileNotOpenException : public SecureConnectionException
{
    public:
    const char *what() const throw()
    {
        return "file is not open";
    }
};

class FileDoesNotExistsException : public SecureConnectionException
{
    public:
    const char *what() const throw()
    {
        return "file does not exists";
    }
};

class InvalidDigitalSignException : public SecureConnectionException
{
    public:
    const char *what() const throw()
    {
        return "Digital signature not valid";
    }
};

class SecureConnection
{
private:
    IClientServerTCP *_csTCP;
    SecureMessageCreator *_sMsgCreator;
    CertificationValidator* _certVal;

    int concatenate(unsigned char* src1, uint32_t len1, unsigned char* src2, uint32_t len2, unsigned char* &dest);
    int readNamesFromFile(const char* filename, std::string* &names);

    void computeSharedKeys(DH *dh_session, BIGNUM *bn);
public:
    SecureConnection(IClientServerTCP *csTCP);

    int sendCertificate(X509* cert);
    int rcvCertificate(X509* &cert);

    void sendSecureMsg(void *buffer, size_t bufferSize);
    int recvSecureMsg(void **plainText);

    void sendAutenticationAndFreshness(unsigned char* expectedMsg, int msgLen, EVP_PKEY* privKey, X509* cert);
    bool recvAutenticationAndVerify(unsigned char* msg,int msgLen);

    void establishConnectionServer();
    void establishConnectionClient();
    
    /**
     * sendFile send a file.
     *
     * @file need an open filestream.
     * @stars if true prints 80 * on the screen
     * @return the fileSize on success.
     * 
     * In case of error Exceptions will be throwed
     * (all right, then. Keep your secret)
     * 
     * ####@###@@@@@@@@@@@@@@@@@@@@#@#@@#####+#++#+#'++######################@@@@@@@#####@@@@@@@@#######@@@##@@@@@@@
     * #+######@@@@@@@@@@@@@@@@@@@@@###@#@#@+'#######'+#########+##########@@@@@@@@@####@@@@@@@@@@@@####@@@@@@@@@@@@
     * +++######@@@@@@@@@##@@@@@@@@@#@##+@##++'++###+'#####'####'###@#####@#@@@@@@@@@@@@@@@@@@@@@@@@@@##@@@@@@@@@@@@
     * #++########@@@@#@#@#@@@@@@@@@@#@''###+;''++#+''#+'::'+##+#+''+;+;##@#@@@##+#@@#@@@@@@@@@@@@###@@#@@@@@@@@@@@@
     * ###########@@@@####@@@@@@@@#@@##+##++'',++;++;+::,,';+;';;::;;:;###'####+##@@#@@@@@@@@@@@@@#@@@@##@@@@@@@@@@@
     * ###+########@@@####@@@@@@@######+++#+:':++';;;:::;+++':::::;;;;:''':;'##@@@#@#@@@@@@@@@@@@@@@@@@#@#@@@@@@@@@@
     * ##++#######@@@@########@@##+'+#'+'+;::::;:'';:,:,;;:;,,,:::,:,::;;''';##@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@##@@@@
     * #########@#@@@#########+;+;';''''',:::,:::;::,,,,,,:,,,:::,:::::;''+;:+'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@##@@@@
     * #++###########@####+###;''+:::::,::::,:;';,:,,::,,,,,,,:,,;,:;::;'+''+++#@####@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
     * #+++#######+#####++'+';:,,,:;::::::,:.,::;,,,,,:;;,,,,::::;::::::;;;;+#'+'#####@#@@@##@@@@@@@@@@@@@@@@@@@@@@@
     * #+++######+++######'':,,:::;:::,,,:.,,,,,.:::::::,';:,::::;;';;''';:'''''##@#####@@@###@@@@@@@@@@@@@@@@@@@@@@
     * ###########++#@###+:,,,;:;::,,,,:::;:,:,::,,::;;;'';':,,;;;;'';;;';::'''+###;####@@@####@@@@@@@@@@@@@@@@@@@@@
     * ###########++#####;.,:':;:,,,::,,::;,:::,:;;:'++'++,;':,';'+'+;;::,::';;+++;+#############@@@@@@@@@@@@@@@@@@@
     * ######'''+##++###',,::::::::,::;:,:;;';.';:;+'+:+++'';+.;+++';;::,,:;:::;':,++;+##+#######@@@@@@@@@@@@@@@@@@@
     * #####+';'++######,,:,,:,,.,:;::;:;';'+,,;+''':;+';##++'.;+++++:::,,,;:':'::,+##+':@@######@@@@@@@@@@@@@@@@@@@
     * ######++########+,,,,,,,,,;;:,,;;:;'':,++',::;,+++;+;;'::+#'+':,,,,:;:':;;:;'++:;+#@@@@@#@@@@@@@@@@@@@@@@@@@@
     * ############+:,,;::,,:,,;;:,.,,;'#+;,+#++#':'+;:,:'':',:;+#+;;,,,::','':::,;'::'#@@#@@@@@@@@@@@@@@@@@@@@@@@@@
     * ###########:'+'';;:::,::,:,::;'++'::++++#;++'+:++:::+::'####'::,,:::;;;'::,:::''++#@#@@@@@@@@@@#@@@@@@@@@@@@@
     * ###++#########+';:,::::::;;,;'''+'###+#+;'++':+#::;++';#+##+;:';:';;;'',;;;';'''''##'#@@##@@@@@@@@@@@@@@@@@@@
     * ###+++######++:,:,,::,;,'';'';':++++#;#:;+'';##:;;#+,:+####'+:+#'+;+'+++#++'''''''''+####@@@@@@@@@@@@@@@@@@@@
     * ++##++#@##@#'::,,,:':::'''++;;''++'+##'##:++##';###;:+####+'+++++++;++####++'#+++'++'++#@##,+#@@@@@@@@@@@@@@@
     * +##@######;;;;::::;:,++++;;:;++#'##@@##;:'++'#++'#';+###@@#++#@##++#'##@@#'++##++++++''##','#@@@@@@@@@@@@@@@@
     * #########'#,;,,,,:','+++;++'+'+##+####''+'+#@######+++###@##+@######+##@##+##+'#++'';++;',++@:#@@@@@@@@@@@@@@
     * @##+#####++';:::;+,:;++#+##++++#####+##+#######@@#@@@@##@@@####@####++###++:######+++'';;###;+##@@@@@@@@@@@@@
     * #########+'',+::;;,';+###+#++##@###############@@#@@@@##@@@@########+#+######++###'+:+''++''+@#@@@@@@@@@@@@@@
     * ########+':;;;,;;';#:++###############@########@@@@@@@##@@@@#@@######+#########@#+''#';+'''##+#@@#@@@@@@@@@@@
     * #########+:;;;:;';;###;##############@#########@##@@@@#@@@@@#@@@@##@############@@#'#+''+;+#+'##@##@@@@@@@@@@
     * #########+:;:.;;;;;+##'+##########################@@@@#@@@@@@@@@@@@@###########@@#####+++;'''##@@##@@@@@@@@@@
     * #########+':,;;;''''+##+######@#####@#############@@@##@@@@@@@@@@@@@@####@@#@####@#@@#+'+';;;+#####@###@@@@@@
     * ++###+++';,:;';';;''''#++########@##@############@@@####@@@@@@@@@@@@@@#####@@@@#@@@@@@+';''''''#@#####@@@@@@@
     * '+++####+::;+::;;'+'';+#+##########@@#####@#############@@@@@@@@@@@@@@@@@#####@@@##@@@##++++;###+@@####@@@@@@
     * ########::;''::;'++'''+############@@###################@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@####+#######@###@@@@@@
     * #######':''++:;;'++#'#+'##########@@#@##################@@@@@@#@@@@@@@@@@@@@@@@@@@@@@@@###':+####@@#+@@@@@@@@
     * #######:+';+',;;''###+;++#########@@@################+##@@@@@@@#@@@@@@@@@@@@@@@@@@@@@@@###+'######'#@@@@@@@@@
     * #++'##;'#';+::,:;'##+;'''##########@###############++######@@@@@@@@@@@@@@@@@@@@@@@@@@@@@#@#++###++#@@@@@@@@@@
     * ++++#'+'',':;::;''+#':'';+##########@#########@##+######@####@@@@@@@@##@@@@@@@@@@@@@@@@##@#++###++@@@@@@@@@@@
     * ++##+'';::';;;:';;+':'''++########+##############+++++########@@#@@@##@@@@@@@@@@@@@@@@@@##@#+++++@@##+#@@@@@@
     * +###+'':;:;:;':';+;;#+''+##++#####++##############++++############@@@@@@@@@@@@@@@@@@@@@###@#+++++'#'#@@@@@@@@
     * +++#+';;::;:';::;;'#''''+##++#####++##############+++++###########@@@@#@@@@@@@@@@@@@@@@######+#+++##+#@@@@@@@
     * +++#+';;:::'+;,,:;';+''+#++'+####++++#+##########@##+++#@##########@#@@##@@@@@@@@@@@@@@@#######';'++##@@@@@@@
     * ##+''+;:,,;+:'';;''';';+'+'+#####++++##+###@######@####@#++#+#######@#@@#####@@@@@@@@@@@#@#######'+#####@@@@@
     * #@#'';;.,;+++;,;''+:;;'''++####++++'''##++###@##########++++++##############@@@@@@@@@@@@@@#####;;#'+;+@@##@@@
     * ####';:;;'''+:,;+++'''''+++''###++''''+###+##########+++++++++##+############@@@@@@@@@@@@@@####';#+'#####@@@@
     * #+'';;;;':;;;:''+++++'''''''''''''''''''++##+######++++++++++++++++#######@@#@@@@@@@@@@@@@@####+'+#+######@@#
     * #+''''+;;:::,,:''+'++'''''''''''''''''''''''''''++++++'++++++++++++#######@@#@@@@@@@@@@@@@@@####+'####@@;#@#@
     * +++''';::,,,,,:'+'';;''';''';''''''''''''''''''''''+'+''''++++++++++#######@@@@@@@@@@@@@@@@#@###+;+###@@#@@#@
     * #+'''';:,,,,,,.:':;;;''';';;''''''''''''''''''''''''''''''++++++++++####@##@@@@@@@@@@@@@@@@@@#@@+++#@###+##@@
     * ++++';:,,,,,,,;:+::'''''';;;'''''''''''''''''''''''''''++'+++++++++++###@##@@@@@@@@@@@@@@#@@##@@#+###@@@#@#;@
     * ;;''';,::,:::,+'++;'''';;;;''';'''''''''''''''''''''''+++++++++++++++######@@@@@@@@@@@@@@@@@@@@@#######@@@###
     * ';''',::;':;;+++++''''''';'''''''''''''''''''''''''+++++++++#++++++++######@@@@@@@@@@@@@@@@@@@@@#######@@@@#@
     * +'':;;:;;;:'+#++#+'''''''''''''';''''''''''''''++++++##########+++++++#####@@@@@@@@@@@@@@@@@@#@@#######@@@@@@
     * ++:''';;::''+#++++';'''''''''''''''''''''''''+++#########++++++#++++++#####@@@@@@@@@@@@@@##@@@@@@#####@@@@@@@
     * +,#+'';;:;''+##+#+';'''''''''''''''''''''''++++#####++++++++++++++++++#####@@@@@@@@@@@@@@@#####@@###+++'+#@@@
     * ;+++'';;;:''+#+##+;;''++++++++''''''''''''+++++++++++++++++++++++++++++####@@@@@@@@@@@@@@@#######@##++;;####@
     * '+#+'':';:''+++#++;++++######++'''''''''''+++++++++++++++++++++++++++++####@@@@@@@@@@@@@@@@@#######@#########
     * '+##';:;;:;;'+#+#';+++++###+++++''''''''''+++++++++++++++++++++++++++++####@@@@@@@@@@@@@@@@#########@####@#@@
     * +##+';:::;,,'####;''++++++++++++''''''''''++++++++++++++++++##+++++++++#####@@@@@@@@@@@@@@@#############@@###
     * +#+'+':::;:,'++##:;'''''++++++++''''''''''+++++++++++++++++++++++++++++#####@@@@@@@@@@@@@@@####@@@#########@@
     * +#+'+',:;:;:;'###;'''''++++++++++'''''''''++++++++++++####++++++++++++++####@@@@@@@@@@@@@@@@#####@########+#@
     * ++''';;;:,,';'###+'+++++++++++++++''''''''++++++++++++##@####+++++++++++####@@@@@@@@@@@@@@@@############++'##
     * ++'++''';::';'####'+++''''+++###++'';''''+++++++#+';';#######++++''''+++#####@@@@@@@@@@@@@@@@#@@@############
     * +++++'+'''+'::+###''++''+########+'';''''++++'++++++++##++++++++'''''+++#####@@@@@@@@@@@@@@@@@@@@@@######+###
     * ####:#+#'';;::++##;'+'+##+'#+@###+'';'''''++++''+''''++++++++++''''''++++#####@@@@@@@@@@@#@@@@@@@@#######++##
     * +#####+';''':;'+##;'''##'''##++++'';;'''''+++++++++'''+++++++++'''''+++++#######@@@@@@@@@###@@@@@@@@######+++
     * +++####++''':;'+##;;:;++++++''++''';;''''''++++++++++++++++++''''''''++++#######@@@@@@@@@####@@@@@@@######+++
     * +++####+''++::'+##;';;'''''''+++''';;''''''+++'''''''''''''''''''''++++++#######@@@@@@@@@####@@@@@@@#######+#
     * ##+###';';;:;''+##;';;'''''''++'''';;'''''''++''''''''''''''''''''+++++++########@@@@@@@@####@@@@@@@#########
     * #####+''+;;#;''###;;;;''''''''''''';;'''''''''''''''''''''''''''''++++++++#######@@@@@@@#####@@@@@@@#######+#
     * #########+;;'''###;;;;;'''''''''''';'''''''''''''''''''''''''''''''+++++++########@#@@@@#####@@@@@@@######+++
     * +#####+#++''+;'++#;;;;;;''''''''''';'''''''''''''''''''''''''''''''+++++++########@###@@#####@@@@@@@######'''
     * ++++++++++'''+'''+:::;;;;;'''''''';;''''''''''''''''''''''''''''''''+++++++#######@#@@@#######@@@@@@@#+###'''
     * +++++++++'';;+'''+:::;;;;;'''''''';;'''''''+++''''''''''''''''''''''+++++++#######@###@#+#####@@@@@@@#####++'
     * ++++++##++;;:'+''':;:;;;'''''''''';;'''''''+++++''''''''''''''''+++'+++++++###########@++#####@@@@@@@@####++#
     * '+++####++';;;++++::::;;''''''''';;;'''''''''++++'''''''''''''''++++++++++++##########@+#####@@@@@@#@#####++#
     * '++++++++';';;+++#::;;;''''''''';;;;''''''''''+++'''''''''''''''++'+++++++++############++##@@@@@@##########+
     * ##+';'+##+'++,:+++::;;'''''''''';;'''''''''''++++'''''''''''''''''''++++++++###########+++#@@@@@@@##+####'+##
     * ##+''#######'':;##;:;'''''''''''''''''''''''+++++''''''''''''''+'++++++++++++###########+#@@@@@@@###+++#+####
     * ###+########+';:'#;:;'''''''''''++''''++###+++++''''''''''''''''++++'++++++++############@@@@@@@@###++##+####
     * ############+;:::#':''''''''''''''''+'++++'++++'''''''''''''''''++++''++++++++###########@@@@@@@@@#+#########
     * ############+:,::+#:'''''''''''''''''+++++'+++''''''''''''''''''++++'+'++++++++#########@#@@@@@@@@#####+#####
     * ############';:::'+;''''''''''''''''''++'''+++''''''''''''''''''''''''''+++++++##########@@@@@@@@@@@#########
     * ############''+:;:'+''''''''''''''''''''''''''''''''''''''''''''''''''''+++++++###########@@@@@@@@#@@#'#####'
     * ###########+'+#'::;+;'''''''''''''''''''''''''''''''''''+'''''''''''''''++++++++###########@@@@@##@@@#+'#+###
     * ###########+++#+::;''''''''''''''''''''''''''''''''''''''''''''''''''''+++++++++########@@@@@@@@####@@#;;###@
     * ############;'#':;';;''''''''''''''''''''''''''''''+''''+''''''''''''''+++++++++#####@@@@@@@@@@@#####@#+'####
     * @@##+####+'+#++;:::;:'''''''''''''''''''''''++++++++++++'''''''++''''''++++++++++++##@@@@@@@##@@@#######+'@##
     * #############'';;:;;'''''''''''''''''++++++++++#####+++++''''''''''''''++++++++++++##@@@@@@@@@@@@@@#@#+###@@@
     * ###############',;;':+'''''''''''+++#++++++#+#+##++++++++'''''+''''''''++++++++++++#@@@@@@@@@@@@@@@###+###@#@
     * ############@##';:,:;''''''''''++++####+######++++++++''''''''+'''''''+++++++++++++#@@@@@@@@@@@@@@#@##+++#@#@
     * ######++++##@##+;',:;+'''''''''''''++++++++#++++++++++'''''''++'''''+++++++++++++++##@@@@@@@@@@@@@##@##+####@
     * ######++#######+++'++''''''''''''''''+++++++++++++++''''''''+++'''''+'+++++++++++++##@@@@@@@@@@@@@###@######@
     * ################++'+#++''''''''''''''''''''++++++++'''''''''++''''''++++++++++++++++#@@@@@@@@@@@@@@@@@#+#+###
     * #####+##++++####'+'+#++'''''''''''''''''''+++++++''''''''''++++'''''+''+++++++++++++#@@@@@@@@##@@@@###'+#####
     * ########+++++++##''+#+++''''''''''''''''++++++'''''''''''''+++'''+''++++++++++++++++#@@@@@@@@####@@@@#++#####
     * #####+##++'++++#+''#+''';'''''''''''''''''+''''''''''''''''+++''''''++++++++++++++++#@@@@@@@@######@#;'+#####
     * #####''##++++++'#+''';:,:'''''''''''''''''''''''''''''''''+++++'''++++++++++++++++++#@@@@@@@@@@#+###@#+######
     * ######+#####++;+##';;::;::''''''''''''''''''''''''''''''''++++++'+++++++++++++++++++#@@@@@@@@@@@####+########
     * ######+######+++''::,:;,,:''''''''''''''''''''''''''''''''++++++++++++++++++++++++++#@@@@@@#@@@@@##+;######++
     * ######++#####;#;++::;::,,::''''''''''''''''''''''''''''''+++++++++++++++++++++++++++#@@@@@@##@@@@#'#+##+##+#;
     * ++#####+######'#+,,:::,:::'#'''''''''''''''''''''''''''''+++++++++++++++++++++++++++#@@@@@@@@+#@@#########+#@
     * #+##############',':,;:,:+#'''''''''''''''''''''''''''''++++++++++++++++++++++++++++#@#@@@@@@@####++#####++@@
     * ################''';:;,:++::'+''''''''''''''''''''''''''++++++++++++++++++++++++++++#@@@@@@@#@@#@#+++#+##'#@@
     * #####@@@########+;',;:;+':;;;+#'''''''''''''''''''''''''++++++++++++++++++++++++++++##@@@@@@#'#@@######'+'##@
     * #################+;::;+'::;;'''+'''''''''''''''''''''''+++++++++++++++++++++++++++++###@@@@@@@#@@@#####++'###
     * ##+###############+;:'';,;;#'''++''''''''''''''''''''++++++++++++++++++++++++++++++++##@@@@@@@@@@@@@#'++'####
     * @##################''+;:;;+#;'+#+;'''''''''''''''''+++++++++++++++++++++++++++++++++++##@@@@@@@@@@@@#';:;;;'#
     * #############+####+#+#';;:#''##+::;'''''''''''''''++++++++++++++++++++++++++++++++++++##@@@@@@@@@@@@@#+'+,,,:
     * ##@@@############++#+++;;+''###;;;'''''''''''''''+++++++++++++++++++++'++++++++++++++++##@@@@@@@@@@@@++##;,,:
     * ###@@@@##############++';+#'##;';;''''''''''''++++++++++++++++++++++++''++++++++++++++++#@@@@@@@#@@@####@#,''
     * ####@@###############+++''#++@'';+''++''++++++++++++++++++++++++++'+'+''+++++++++++++++++#@@@@@@@@@@####@#;++
     * ################+###++++'++#'##'++;+''''''''''++++++++++++++++''''''''''++++++++++++++++++#@@@@@@@@@#@@@##+++
     * #####################++'''+###@#''':'+;'''''''''''++++'++'++''''''''''''''+++++++++++++++++#@@@@@@@@#@##+++++
     * ######+''+#######+##+++';'+######+'++'''''''''''''++''''''''''''''''''''+++++++++++++++++++#@@@@@@@@@#+###+++
     * ##@@##+''++++++++'++++'++#+++#####+#+'''''''''''''''''''''''''''''''''''++++++++++++++++++++#@@@@@@@#+####+++
     * '#####++'';''++++++++++++###+'+#####++'+''''''''''''''''''''''''''''''''+++++++++++++++++++++#@@@@##+####++++
     * #+'+####+''''+++##++#+++++@#+++'#####++#''''''''''''''''''''''''''''''''++++++++++++++++++++++@@#+##########+
     * '##+;'###+'''+########+##';:,,,,,,'+##'++'''''''''''''''''''''''''''''''+++++++++++++++++++++++++############
     * '''###+'++';'######++##;,..,,,,;;;;:;;,::''''''''''''''''''''''''''''''''++'+++++++++++++++++#####+##########
     * #';;'+##'...'+###+++',..,:::'+';+'+;,,:;''''''''''''''''''''''''''''''''''++++++++++++++++++#################
     * +:++++;.,:++++####;,.:;++###++++':,,;+#++'''''''''''''''''''''''''''''''''+++++++++++++++++##################
     * +##+:,,,+###++##+,,,:'++++++#';,,:;++##'#+''''''''''''''''''''''''''''''''+++++++++++++++####################
     * ###+:::####+##+:,,.:;;'++#+':,,,;+####:+#+'''''''''''''''''''''''''''''''''++++++++++++######################
     * #@##::+###++++,.,,;'+##+',.,,,;+####+,:+#+;''''''''''''''''''''''''''''''''+++++++++++#####################++
     * ####;##@#+'+',.:''####+,,,,,,;++###+,.:###'''''''''''''''''''''''''''''''''++++++++++###################+++++
     * @@######+++'.:+####@#;.,,,,:'#####+,.,;+##+''''''''''''''''''''''''''''''''+++++++++################+++++++++
     * @###@##+++;.'#####@#;,,,,,:'+#####,.,:'####'''''''''''''''''''''''''''''''+++++++++################++++++++++
     * ####@#+++',+###@@@#:..,,,:+######:.,,;+####'''''''''''''''''''''''''''''''++++++++##############++#++++#+++++
     * ######+++''###@@#+'.,,,,:+######'..,,;+####'''''''''''''''''''''''''''''''++++++###############+####+++++++++
     * #############@####,,;+:,'######+;,..:'#####+'''''''''''''''''''''''''''''++++++#################+####++++++++
     * ############@####','#+;:+#####++'::.:+#+####''''''''''''''''''''''''''''++++++###################++++++++++++
     * #####@#####@####+::+#+;;#####++;:::,;+++####''''''''''''''''''''''''''''+'+++############+#######+++#+++#++++
     * ####@@@####@###++,+##+;+#####+':::,,,'''+###''''''''''''''''''''''''''''''++#########+##############+++++++++
     * ####@@####@#####;;###++######+;::.::;,'+####+''''''''''''''''''''''''''''+++########++#######+###+###+#++++++
     * #@@@#####@@##+##,+###+#######;::,,'+,;'+#####;''''''''''''''''''''''''''+++#########+###+####+#++###+####++++
    */
    int sendFile(std::ifstream &file, bool stars);
    int receiveFile(const char *filename, bool stars);
    int reciveAndPrintBigMessage();

    
};