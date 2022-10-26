LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libShadowSocksConsole

LOCAL_SRC_FILES  := \
			./libsysproxy.c \
			./main.cpp \
			./TCPChecker.cpp \
			./ShadowSocks.cpp \
			./Tun2Socks.cpp \
			./SSServer.cpp \
			./SSClient.cpp \
			./ConsoleLoop.cpp \
			./ShadowSocksController.cpp \
			\
			./wwwsrc/WebUI.cpp \
			./wwwsrc/WebUIServers.cpp \
			./wwwsrc/WebUISettings.cpp \
			./wwwsrc/WebUITasks.cpp \
			./wwwsrc/WebUIVPN.cpp \
			./wwwsrc/WebUIRuns.cpp \
			\
			../../Addon/WGetDownloader.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParser.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParserHttp.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParserMultiplex.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParserSocks5.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParserTunnel.cpp \
			\
			./www/textfill.cpp \
			\
			../../Addon/httpsrv/HttpHostRouter.cpp \
			../../Addon/httpsrv/HttpPathRouter.cpp \
			../../Addon/httpsrv/HttpRequest.cpp \
			../../Addon/httpsrv/HttpServer.cpp \
			\
			../../Addon/sha1.cpp \
			\
			../../version.cpp \
			../../DPLibConfig.cpp \
			../../_Driver/Application.cpp \
			../../_Driver/Files.cpp \
			../../_Driver/Path.cpp \
			../../_Driver/Service_lin.cpp \
			../../_Driver/ThreadWorker.cpp \
			\
			../../Converter/Converter.cpp \
			../../Converter/Base64.cpp \
			\
			../../Crypt/Crypt.cpp \
			../../Crypt/SCH/System.cpp \
			../../Crypt/SCH/v1/Crypt_v1.cpp \
			../../Crypt/SCH/v1/Key_v1.cpp \
			../../Crypt/SCH/v1/UniKey_v1.cpp \
			../../Crypt/Converters/PrimitiveConverter.cpp \
			\
			../../Log/stlog.cpp \
			\
			../../Network/TCPClient.cpp \
			../../Network/TCPServer.cpp \
			../../Network/UDPClient.cpp \
			../../Network/UDPServer.cpp \
			../../Network/Utils.cpp \
			../../Network/libSimpleDNS/dns.cpp \
			../../Network/proxy/Chain.cpp \
			../../Network/proxy/Connector.cpp \
			../../Network/proxy/ConnectorDirect.cpp \
			../../Network/proxy/ConnectorHttpProxy.cpp \
			../../Network/proxy/ConnectorSocks.cpp \
			../../Network/DNS/DNSClient.cpp \
			../../Network/DNS/DNSNetworkClient.cpp \
			../../Network/DNS/DNSRequestResponseStruct.cpp \
			../../Network/DNS/DNSResponse.cpp \
			\
			../../Parser/ArgumentParser.cpp \
			../../Parser/SmartParser.cpp \
			../../Parser/Setting.cpp \
			../../Types/Random.cpp \
			../../Types/SmartPtr.cpp
			
include $(BUILD_EXECUTABLE)
#include $(BUILD_SHARED_LIBRARY)
