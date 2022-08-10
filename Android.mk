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
			../../Addon/libproxy/Chain.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParser.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParserHttp.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParserMultiplex.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParserSocks5.cpp \
			../../Addon/libproxy/ProtoParsers/SocketParserTunnel.cpp \
			../../Addon/libproxy/Connectors/Connector.cpp \
			../../Addon/libproxy/Connectors/ConnectorDirect.cpp \
			../../Addon/libproxy/Connectors/ConnectorHttpProxy.cpp \
			../../Addon/libproxy/Connectors/ConnectorSocks.cpp \
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
			../../Converter/Converter.cpp \
			../../Crypt/Crypt.cpp \
			../../Crypt/SCH/System.cpp \
			../../Crypt/SCH/v1/Crypt_v1.cpp \
			../../Crypt/SCH/v1/Key_v1.cpp \
			../../Crypt/SCH/v1/UniKey_v1.cpp \
			../../Crypt/Converters/PrimitiveConverter.cpp \
			../../Log/stlog.cpp \
			../../DPLibConfig.cpp \
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
