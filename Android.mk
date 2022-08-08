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
			./sha1.cpp \
			\
			./wwwsrc/WebUI.cpp \
			./wwwsrc/WebUIServers.cpp \
			./wwwsrc/WebUISettings.cpp \
			./wwwsrc/WebUITasks.cpp \
			./wwwsrc/WebUIVPN.cpp \
			./wwwsrc/WebUIRuns.cpp \
			\
			./WGetDownloader/Downloader.cpp \
			./WGetDownloader/libproxy/Chain.cpp \
			./WGetDownloader/libproxy/ProtoParsers/SocketParser.cpp \
			./WGetDownloader/libproxy/ProtoParsers/SocketParserHttp.cpp \
			./WGetDownloader/libproxy/ProtoParsers/SocketParserMultiplex.cpp \
			./WGetDownloader/libproxy/ProtoParsers/SocketParserSocks5.cpp \
			./WGetDownloader/libproxy/ProtoParsers/SocketParserTunnel.cpp \
			./WGetDownloader/libproxy/Connectors/Connector.cpp \
			./WGetDownloader/libproxy/Connectors/ConnectorDirect.cpp \
			./WGetDownloader/libproxy/Connectors/ConnectorHttpProxy.cpp \
			./WGetDownloader/libproxy/Connectors/ConnectorSocks.cpp \
			\
			./www/textfill.cpp \
			\
			../../Addon/httpsrv/HttpHostRouter.cpp \
			../../Addon/httpsrv/HttpPathRouter.cpp \
			../../Addon/httpsrv/HttpRequest.cpp \
			../../Addon/httpsrv/HttpServer.cpp \
			\
			../../version.cpp \
			../../DPLibConfig.cpp \
			../../_Driver/Application.cpp \
			../../_Driver/Files.cpp \
			../../_Driver/Path.cpp \
			../../_Driver/Service_lin.cpp \
			../../_Driver/ThreadWorker.cpp \
			../../Converter/Converter.cpp \
			../../Crypt/Crypt.cpp \
			../../Crypt/SCH/System.cpp \
			../../Crypt/SCH/v1/Crypt_v1.cpp \
			../../Crypt/SCH/v1/Key_v1.cpp \
			../../Crypt/SCH/v1/UniKey_v1.cpp \
			../../Crypt/Converters/PrimitiveConverter.cpp \
			../../Log/stlog.cpp \
			../../Network/TCPClient.cpp \
			../../Network/TCPServer.cpp \
			../../Network/Utils.cpp \
			../../Network/libSimpleDNS/dns.cpp \
			../../Types/MemoryManager.cpp \
			../../Parser/ArgumentParser.cpp \
			../../Parser/SmartParser.cpp \
			../../Parser/Setting.cpp \
			../../Types/Random.cpp \
			../../Types/SmartPtr.cpp
			
include $(BUILD_EXECUTABLE)
#include $(BUILD_SHARED_LIBRARY)
