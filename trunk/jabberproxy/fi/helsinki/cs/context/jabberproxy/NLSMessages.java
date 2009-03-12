package fi.helsinki.cs.context.jabberproxy;

import org.eclipse.osgi.util.NLS;

public class NLSMessages extends NLS {

    private static final String BUNDLE_NAME = "fi.helsinki.cs.context.jabberproxy.messages"; //$NON-NLS-1$

    private NLSMessages() {
    }

    static {
        // initialize resource bundle
        NLS.initializeMessages(BUNDLE_NAME, NLSMessages.class);
    }
    public static String ContextEventHandler_WRITER_FROM_SOCKET;
    public static String ContextEventHandler_EMPTYSTRING;
    public static String ContextEventHandler_XMLHEADER_STREAM;
    public static String ContextEventHandler_WRITE_TO_SOCKET;
    public static String ContextEventHandler_ENDSTREAM;
    public static String ContextEventHandler_STREAM;
    public static String ContextEventHandler_XMLHEADER;
    public static String ContextEventHandler_IDENT;
    public static String ContextEventHandler_TUPLE;
    public static String ContextEventHandler_TAG_BEGIN_TUPLE;
    public static String ContextEventHandler_TAGEND;
    public static String ContextEventHandler_TS;
    public static String ContextEventHandler_ID;
    public static String ContextEventHandler_NAME;
    public static String ContextEventHandler_IMEI;
    public static String ContextEventHandler_TAGBEGIN;
    public static String ContextEventHandler_NAMETAG;
    public static String ContextEventHandler_ENDNAMETAG_IMEITAG;
    public static String ContextEventHandler_ENDIMEITAG;
    public static String ContextEventHandler_ENDTUPLETAG;
    public static String ContextEventHandler_BEGINTAG_SLASH;
    public static String ContextEventHandler_ACK_ID;
    public static String ContextEventHandler_ENDID_ENDACK;
    public static String ContextEventHandler_EQUALSQUOTE;
    public static String ContextEventHandler_QUOTE;
    public static String XMLFilterThread_CLOSE_SOCKET;
    public static String XMLFilterThread_READ_FROM_SOCKET;
    public static String XMLFilterThread_PARSE_XML_STREAM;
    public static String ContextProxy_SOCKET_OPERATION_FAILED;
    public static String ContextProxy_ACCEPT_CONNECTION;
    public static String ContextProxy_SUPPLY_PORT_NUMBER;
    public static String XMLFilterThread_UNABLE_TO_GET_READER;
    public static String ContextEventHandler_ENDCONTEXT;
    public static String ContextEventHandler_CHARACTERS_OUTSIDE_EVENT_OR_TUPLE;
    public static String ContextEventHandler_SPACE;
    public static String ContextEventHandler_NAME_NOT_BOUND;
    public static String ContextEventHandler_REMOTEEXCEPTION;
    public static String ContextEventHandler_MALFORMED_URL;
    public static String ContextProxy_ILLEGAL_NUMBER_FORMAT;
    public static String ContextProxy_DEFAULTDATEFORMAT;
    public static String ContextProxy_DEFAULTADDURL;
    public static String ContextEventHandler_0;
    public static String ContextEventHandler_NEWLINE;
    public static String ContextEventHandler_2;
    public static String ContextEventHandler_3;
	public static String JabberEventHandler_NEWLINE_XMLNSCOLON;
	public static String JabberEventHandler_NEWLINE_XMLNS;
	public static String JabberEventHandler_KEEPALIVE;
	public static String JabberEventHandler_AMP;
	public static String JabberEventHandler_AMPESC;
	public static String JabberEventHandler_LT;
	public static String JabberEventHandler_LTESC;
	public static String ContextEventHandler_NEWLINEXMLNSCOL;
	public static String ContextEventHandler_NEWLINEXMLNS;
	public static String ContextEventHandler_KEEPALIVE;
	public static String ContextEventHandler_DATEFORMAT;
	public static String ContextEventHandler_TIMEZONE;
	public static String ContextEventHandler_REGEX;
	public static String ContextEventHandler_PRESENCE;
	public static String ContextEventHandler_AMP;
	public static String ContextEventHandler_AMPESC;
	public static String ContextEventHandler_LT;
	public static String ContextEventHandler_LTESC;
	public static String ContextEventHandler_QUOT;
	public static String ContextEventHandler_QUOTESC;
}
