#include <gtest/gtest.h>

#include "script.h"
#include "test/test_ddf.h"

#include <dlib/configfile.h>
#include <dlib/dstrings.h>
#include <dlib/hash.h>
#include <dlib/log.h>
#include <dlib/time.h>
#include <dlib/socket.h>
#include <dlib/thread.h>
#include <dlib/web_server.h>

extern "C"
{
#include <lua/lauxlib.h>
#include <lua/lualib.h>
}

#define PATH_FORMAT "build/default/src/test/%s"

dmhash_t ResolvePathCallback(uintptr_t user_data, const char* path, uint32_t path_size)
{
    return dmHashBuffer64(path, path_size);
}

void GetURLCallback(lua_State* L, dmMessage::URL* url)
{
    lua_getglobal(L, "__default_url");
    *url = *dmScript::CheckURL(L, -1);
    lua_pop(L, 1);
}

uintptr_t GetUserDataCallback(lua_State* L)
{
    lua_getglobal(L, "__default_url");
    uintptr_t default_url = (uintptr_t)dmScript::CheckURL(L, -1);
    lua_pop(L, 1);
    return default_url;
}

class ScriptHttpTest : public ::testing::Test
{
public:
    int m_HttpResponseCount;
    dmWebServer::HServer m_WebServer;
    uint16_t m_WebServerPort;
    dmScript::HContext m_ScriptContext;
    lua_State* L;
    dmMessage::URL m_DefaultURL;
    dmConfigFile::HConfig m_ConfigFile;

protected:

    static void Handler(void* user_data, dmWebServer::Request* request)
    {
        char buf[1024];

        if (strcmp(request->m_Resource, "/") == 0) {
            if (strcmp(request->m_Method, "GET") == 0) {
                const char* a = dmWebServer::GetHeader(request, "X-A");
                const char* b = dmWebServer::GetHeader(request, "X-B");
                if (a && b) {
                    DM_SNPRINTF(buf, sizeof(buf), "Hello %s%s", a, b);
                } else {
                    DM_SNPRINTF(buf, sizeof(buf), "Hello");
                }
                dmWebServer::Send(request, buf, strlen(buf));
            } else {
                // POST
                uint32_t received = 0;
                dmWebServer::Result r = dmWebServer::Receive(request, buf, request->m_ContentLength, &received);
                if (r == dmWebServer::RESULT_OK) {
                    dmWebServer::Send(request, "PONG ", 4);
                    dmWebServer::Send(request, buf, received);
                }
            }
        } else if (strcmp(request->m_Resource, "/sleep") == 0) {
            dmTime::Sleep(2000 * 1000);
        } else {
            dmWebServer::SetStatusCode(request, 404);
        }
    }

    virtual void SetUp()
    {
        dmConfigFile::Result r = dmConfigFile::Load("src/test/test.config", 0, 0, &m_ConfigFile);
        ASSERT_EQ(dmConfigFile::RESULT_OK, r);

        m_HttpResponseCount = 0;
        L = lua_open();
        luaL_openlibs(L);
        m_ScriptContext = dmScript::NewContext(m_ConfigFile, 0);
        dmScript::ScriptParams params;
        params.m_Context = m_ScriptContext;
        params.m_ResolvePathCallback = ResolvePathCallback;
        params.m_GetURLCallback = GetURLCallback;
        params.m_GetUserDataCallback = GetUserDataCallback;
        dmScript::Initialize(L, params);

        ASSERT_EQ(dmMessage::RESULT_OK, dmMessage::NewSocket("default_socket", &m_DefaultURL.m_Socket));
        m_DefaultURL.m_Path = dmHashString64("default_path");
        m_DefaultURL.m_Fragment = dmHashString64("default_fragment");
        dmScript::PushURL(L, m_DefaultURL);
        lua_setglobal(L, "__default_url");

        dmWebServer::NewParams web_params;
        dmWebServer::Result wr = dmWebServer::New(&web_params, &m_WebServer);
        ASSERT_EQ(dmWebServer::RESULT_OK, wr);
        dmWebServer::HandlerParams handler_params;
        handler_params.m_Handler = &Handler;
        handler_params.m_Userdata = this;
        dmWebServer::AddHandler(m_WebServer, "/", &handler_params);
        dmSocket::Address address;
        dmWebServer::GetName(m_WebServer, &address, &m_WebServerPort);
    }

    virtual void TearDown()
    {
        dmWebServer::Delete(m_WebServer);
        if (m_DefaultURL.m_Socket) {
            dmMessage::DeleteSocket(m_DefaultURL.m_Socket);
        }
        dmScript::Finalize(L, m_ScriptContext);
        lua_close(L);
        dmScript::DeleteContext(m_ScriptContext);
        dmConfigFile::Delete(m_ConfigFile);
    }
};

bool RunFile(lua_State* L, const char* filename)
{
    char path[64];
    DM_SNPRINTF(path, 64, PATH_FORMAT, filename);
    if (luaL_dofile(L, path) != 0)
    {
        dmLogError("%s", lua_tolstring(L, -1, 0));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

bool RunString(lua_State* L, const char* script)
{
    if (luaL_dostring(L, script) != 0)
    {
        dmLogError("%s", lua_tolstring(L, -1, 0));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

void DispatchCallbackDDF(dmMessage::Message *message, void* user_ptr)
{
    ScriptHttpTest* test = (ScriptHttpTest*) user_ptr;
    test->m_HttpResponseCount++;
    lua_State* L = test->L;
    assert(message->m_Descriptor != 0);
    dmDDF::Descriptor* descriptor = (dmDDF::Descriptor*)message->m_Descriptor;

    int ref = message->m_Receiver.m_Function - 2;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
    lua_gc(L, LUA_GCCOLLECT, 0);

    dmScript::PushDDF(L, descriptor, (const char*)&message->m_Data[0]);
    int ret = lua_pcall(L, 1, 0, 0);
    if (ret != 0) {
        dmLogError("Error: %s", lua_tostring(L,-1));
        lua_pop(L, 1);
        ASSERT_TRUE(0);
    }
}

TEST_F(ScriptHttpTest, TestPost)
{
    int top = lua_gettop(L);

    ASSERT_TRUE(RunFile(L, "test_http.luac"));

    char buf[1024];
    DM_SNPRINTF(buf, sizeof(buf), "PORT = %d\n", m_WebServerPort);
    RunString(L, buf);

    lua_getglobal(L, "functions");
    ASSERT_EQ(LUA_TTABLE, lua_type(L, -1));
    lua_getfield(L, -1, "test_http");
    ASSERT_EQ(LUA_TFUNCTION, lua_type(L, -1));
    int result = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (result == LUA_ERRRUN)
    {
        dmLogError("Error running script: %s", lua_tostring(L,-1));
        ASSERT_TRUE(false);
        lua_pop(L, 1);
    }
    else
    {
        ASSERT_EQ(0, result);
    }
    lua_pop(L, 1);

    uint64_t start = dmTime::GetTime();
    while (1) {
        dmWebServer::Update(m_WebServer);
        dmMessage::Dispatch(m_DefaultURL.m_Socket, DispatchCallbackDDF, this);

        lua_getglobal(L, "requests_left");
        int requests_left = lua_tointeger(L, -1);
        lua_pop(L, 1);

        if (requests_left == 0) {
            break;
        }

        dmTime::Sleep(10 * 1000);

        uint64_t now = dmTime::GetTime();
        uint64_t elapsed = now - start;
        if (elapsed / 1000000 > 4) {
            ASSERT_TRUE(0);
        }
    }

    ASSERT_EQ(top, lua_gettop(L));
}

TEST_F(ScriptHttpTest, TestTimeout)
{
    int top = lua_gettop(L);

    ASSERT_TRUE(RunFile(L, "test_http_timeout.luac"));

    char buf[1024];
    DM_SNPRINTF(buf, sizeof(buf), "PORT = %d\n", m_WebServerPort);
    RunString(L, buf);

    lua_getglobal(L, "functions");
    ASSERT_EQ(LUA_TTABLE, lua_type(L, -1));
    lua_getfield(L, -1, "test_http_timeout");
    ASSERT_EQ(LUA_TFUNCTION, lua_type(L, -1));
    int result = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (result == LUA_ERRRUN)
    {
        dmLogError("Error running script: %s", lua_tostring(L,-1));
        ASSERT_TRUE(false);
        lua_pop(L, 1);
    }
    else
    {
        ASSERT_EQ(0, result);
    }
    lua_pop(L, 1);

    uint64_t start = dmTime::GetTime();
    while (1) {
        dmWebServer::Update(m_WebServer);
        dmMessage::Dispatch(m_DefaultURL.m_Socket, DispatchCallbackDDF, this);

        lua_getglobal(L, "requests_left");
        int requests_left = lua_tointeger(L, -1);
        lua_pop(L, 1);

        if (requests_left == 0) {
            break;
        }

        dmTime::Sleep(10 * 1000);

        uint64_t now = dmTime::GetTime();
        uint64_t elapsed = now - start;
        if (elapsed / 1000000 > 4) {
            ASSERT_TRUE(0);
        }
    }

    ASSERT_EQ(top, lua_gettop(L));
}

TEST_F(ScriptHttpTest, TestDeletedSocket)
{
    int top = lua_gettop(L);

    ASSERT_TRUE(RunFile(L, "test_http.luac"));

    char buf[1024];
    DM_SNPRINTF(buf, sizeof(buf), "PORT = %d\n", m_WebServerPort);
    RunString(L, buf);

    lua_getglobal(L, "functions");
    ASSERT_EQ(LUA_TTABLE, lua_type(L, -1));
    lua_getfield(L, -1, "test_http");
    ASSERT_EQ(LUA_TFUNCTION, lua_type(L, -1));
    int result = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (result == LUA_ERRRUN)
    {
        dmLogError("Error running script: %s", lua_tostring(L,-1));
        ASSERT_TRUE(false);
        lua_pop(L, 1);
    }
    else
    {
        ASSERT_EQ(0, result);
    }
    lua_pop(L, 1);

    dmMessage::DeleteSocket(m_DefaultURL.m_Socket);
    m_DefaultURL.m_Socket = 0;

    for (int i = 0; i < 10; ++i) {
        dmWebServer::Update(m_WebServer);
        dmTime::Sleep(10 * 1000);
    }

    ASSERT_EQ(top, lua_gettop(L));
}

int main(int argc, char **argv)
{
    dmSocket::Initialize();
    dmDDF::RegisterAllTypes();
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    dmSocket::Finalize();
    return ret;
}
