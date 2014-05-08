#include <gtest/gtest.h>

#include "../gui.h"
#include "../gui_script.h"

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
}

static const float EPSILON = 0.000001f;

static const float TEXT_GLYPH_WIDTH = 1.0f;
static const float TEXT_MAX_ASCENT = 0.75f;
static const float TEXT_MAX_DESCENT = 0.25f;

void GetTextMetricsCallback(const void* font, const char* text, float width, bool line_break, dmGui::TextMetrics* out_metrics);

class dmGuiScriptTest : public ::testing::Test
{
public:
    dmScript::HContext m_ScriptContext;
    dmGui::HContext m_Context;

    virtual void SetUp()
    {
        m_ScriptContext = dmScript::NewContext(0, 0);

        dmGui::NewContextParams context_params;
        context_params.m_ScriptContext = m_ScriptContext;
        context_params.m_GetTextMetricsCallback = GetTextMetricsCallback;
        m_Context = dmGui::NewContext(&context_params);
    }

    virtual void TearDown()
    {
        dmGui::DeleteContext(m_Context, m_ScriptContext);
        dmScript::DeleteContext(m_ScriptContext);
    }
};

void GetTextMetricsCallback(const void* font, const char* text, float width, bool line_break, dmGui::TextMetrics* out_metrics)
{
    out_metrics->m_Width = strlen(text) * TEXT_GLYPH_WIDTH;
    out_metrics->m_MaxAscent = TEXT_MAX_ASCENT;
    out_metrics->m_MaxDescent = TEXT_MAX_DESCENT;
}

TEST_F(dmGuiScriptTest, URLOutsideFunctions)
{
    dmGui::HScript script = NewScript(m_Context);

    const char* src = "local url = msg.url(\"test\")\n";
    dmGui::Result result = SetScript(script, src, strlen(src), "dummy_source");
    ASSERT_EQ(dmGui::RESULT_SCRIPT_ERROR, result);
    DeleteScript(script);
}

TEST_F(dmGuiScriptTest, GetScreenPos)
{
    dmGui::HScript script = NewScript(m_Context);

    dmGui::NewSceneParams params;
    params.m_MaxNodes = 64;
    params.m_MaxAnimations = 32;
    params.m_UserData = this;
    dmGui::HScene scene = dmGui::NewScene(m_Context, &params);
    dmGui::SetSceneScript(scene, script);

    const char* src = "function init(self)\n"
                      "    local p = vmath.vector3(10, 10, 0)\n"
                      "    local s = vmath.vector3(20, 20, 0)\n"
                      "    local n1 = gui.new_box_node(p, s)\n"
                      "    local n2 = gui.new_text_node(p, \"text\")\n"
                      "    gui.set_size(n2, s)\n"
                      "    assert(gui.get_screen_position(n1) == gui.get_screen_position(n2))\n"
                      "end\n";
    dmGui::Result result = SetScript(script, src, strlen(src), "dummy_source");
    ASSERT_EQ(dmGui::RESULT_OK, result);

    ASSERT_EQ(dmGui::RESULT_OK, dmGui::InitScene(scene));

    dmGui::DeleteScene(scene);

    DeleteScript(script);
}

#define REF_VALUE "__ref_value"

int TestRef(lua_State* L)
{
    lua_getglobal(L, REF_VALUE);
    int* ref = (int*)lua_touserdata(L, -1);
    dmScript::GetInstance(L);
    *ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
    return 0;
}

TEST_F(dmGuiScriptTest, TestInstanceCallback)
{
    lua_State* L = dmGui::GetLuaState(m_Context);

    lua_register(L, "test_ref", TestRef);

    int ref = LUA_NOREF;

    lua_pushlightuserdata(L, &ref);
    lua_setglobal(L, REF_VALUE);

    dmGui::HScript script = NewScript(m_Context);

    dmGui::NewSceneParams params;
    params.m_MaxNodes = 64;
    params.m_MaxAnimations = 32;
    params.m_UserData = this;
    dmGui::HScene scene = dmGui::NewScene(m_Context, &params);
    dmGui::SetSceneScript(scene, script);

    const char* src =
            "function init(self)\n"
            "    test_ref()\n"
            "end\n";

    dmGui::Result result = SetScript(script, src, strlen(src), "dummy_source");
    ASSERT_EQ(dmGui::RESULT_OK, result);

    dmGui::InitScene(scene);

    ASSERT_NE(ref, LUA_NOREF);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    dmScript::SetInstance(L);
    ASSERT_TRUE(dmScript::IsInstanceValid(L));

    dmGui::DeleteScene(scene);

    dmGui::DeleteScript(script);

    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    dmScript::SetInstance(L);
    ASSERT_FALSE(dmScript::IsInstanceValid(L));
}

#undef REF_VALUE

TEST_F(dmGuiScriptTest, TestGlobalNodeFail)
{
    dmGui::HScript script = NewScript(m_Context);

    dmGui::NewSceneParams params;
    params.m_MaxNodes = 64;
    params.m_MaxAnimations = 32;
    params.m_UserData = this;
    dmGui::HScene scene = dmGui::NewScene(m_Context, &params);
    dmGui::HScene scene2 = dmGui::NewScene(m_Context, &params);
    dmGui::SetSceneScript(scene, script);
    dmGui::SetSceneScript(scene2, script);

    const char* src =
            "local n = nil\n"
            ""
            "function init(self)\n"
            "    n = gui.new_box_node(vmath.vector3(1, 1, 1), vmath.vector3(1, 1, 1))\n"
            "end\n"
            ""
            "function update(self, dt)\n"
            "    -- should produce lua error since update is called with another scene\n"
            "    assert(gui.get_position(n).x == 1)\n"
            "end\n";

    dmGui::Result result = SetScript(script, src, strlen(src), "dummy_source");
    ASSERT_EQ(dmGui::RESULT_OK, result);

    result = dmGui::InitScene(scene);
    ASSERT_EQ(dmGui::RESULT_OK, result);

    result = dmGui::UpdateScene(scene2, 1.0f / 60);
    ASSERT_NE(dmGui::RESULT_OK, result);

    dmGui::DeleteScene(scene);
    dmGui::DeleteScene(scene2);

    dmGui::DeleteScript(script);
}

TEST_F(dmGuiScriptTest, TestParenting)
{
    dmGui::HScript script = NewScript(m_Context);

    dmGui::NewSceneParams params;
    params.m_MaxNodes = 64;
    params.m_MaxAnimations = 32;
    params.m_UserData = this;
    dmGui::HScene scene = dmGui::NewScene(m_Context, &params);
    dmGui::SetSceneScript(scene, script);

    const char* src =
            "function init(self)\n"
            "    local parent = gui.new_box_node(vmath.vector3(1, 1, 1), vmath.vector3(1, 1, 1))\n"
            "    local child = gui.new_box_node(vmath.vector3(1, 1, 1), vmath.vector3(1, 1, 1))\n"
            "    assert(gui.get_parent(child) == nil)\n"
            "    gui.set_parent(child, parent)\n"
            "    assert(gui.get_parent(child) == parent)\n"
            "    gui.set_parent(child, nil)\n"
            "    assert(gui.get_parent(child) == nil)\n"
            "end\n";

    dmGui::Result result = SetScript(script, src, strlen(src), "dummy_source");
    ASSERT_EQ(dmGui::RESULT_OK, result);

    result = dmGui::InitScene(scene);
    ASSERT_EQ(dmGui::RESULT_OK, result);

    dmGui::DeleteScene(scene);

    dmGui::DeleteScript(script);
}

TEST_F(dmGuiScriptTest, TestGetIndex)
{
    dmGui::HScript script = NewScript(m_Context);

    dmGui::NewSceneParams params;
    params.m_MaxNodes = 64;
    params.m_MaxAnimations = 32;
    params.m_UserData = this;
    dmGui::HScene scene = dmGui::NewScene(m_Context, &params);
    dmGui::SetSceneScript(scene, script);

    const char* src =
            "function init(self)\n"
            "    local parent = gui.new_box_node(vmath.vector3(1, 1, 1), vmath.vector3(1, 1, 1))\n"
            "    local child = gui.new_box_node(vmath.vector3(1, 1, 1), vmath.vector3(1, 1, 1))\n"
            "    assert(gui.get_index(parent) == 0)\n"
            "    assert(gui.get_index(child) == 1)\n"
            "    gui.move_above(parent, nil)\n"
            "    assert(gui.get_index(parent) == 1)\n"
            "    assert(gui.get_index(child) == 0)\n"
            "    gui.set_parent(child, parent)\n"
            "    assert(gui.get_index(parent) == 0)\n"
            "    assert(gui.get_index(child) == 0)\n"
            "    gui.set_parent(child, nil)\n"
            "    assert(gui.get_index(parent) == 0)\n"
            "    assert(gui.get_index(child) == 1)\n"
            "end\n";

    dmGui::Result result = SetScript(script, src, strlen(src), "dummy_source");
    ASSERT_EQ(dmGui::RESULT_OK, result);

    result = dmGui::InitScene(scene);
    ASSERT_EQ(dmGui::RESULT_OK, result);

    dmGui::DeleteScene(scene);

    dmGui::DeleteScript(script);
}

TEST_F(dmGuiScriptTest, TestCloneTree)
{
    dmGui::HScript script = NewScript(m_Context);

    dmGui::NewSceneParams params;
    params.m_MaxNodes = 64;
    params.m_MaxAnimations = 32;
    params.m_UserData = this;
    dmGui::HScene scene = dmGui::NewScene(m_Context, &params);
    dmGui::SetSceneScript(scene, script);

    const char* src =
            "function init(self)\n"
            "    local n1 = gui.new_box_node(vmath.vector3(1, 1, 1), vmath.vector3(1, 1, 1))\n"
            "    gui.set_id(n1, \"n1\")\n"
            "    local n2 = gui.new_box_node(vmath.vector3(2, 2, 2), vmath.vector3(1, 1, 1))\n"
            "    gui.set_id(n2, \"n2\")\n"
            "    local n3 = gui.new_box_node(vmath.vector3(3, 3, 3), vmath.vector3(1, 1, 1))\n"
            "    gui.set_id(n3, \"n3\")\n"
            "    local n4 = gui.new_text_node(vmath.vector3(3, 3, 3), \"TEST\")\n"
            "    gui.set_id(n4, \"n4\")\n"
            "    gui.set_parent(n2, n1)\n"
            "    gui.set_parent(n3, n2)\n"
            "    gui.set_parent(n4, n3)\n"
            "    local t = gui.clone_tree(n1)\n"
            "    assert(gui.get_position(t.n1) == gui.get_position(n1))\n"
            "    assert(gui.get_position(t.n2) == gui.get_position(n2))\n"
            "    assert(gui.get_position(t.n3) == gui.get_position(n3))\n"
            "    assert(gui.get_text(t.n4) == gui.get_text(n4))\n"
            "    gui.set_position(t.n1, vmath.vector3(4, 4, 4))\n"
            "    assert(gui.get_position(t.n1) ~= gui.get_position(n1))\n"
            "    gui.set_text(t.n4, \"TEST2\")\n"
            "    assert(gui.get_text(t.n4) ~= gui.get_text(n4))\n"
            "end\n";

    dmGui::Result result = SetScript(script, src, strlen(src), "dummy_source");
    ASSERT_EQ(dmGui::RESULT_OK, result);

    result = dmGui::InitScene(scene);
    ASSERT_EQ(dmGui::RESULT_OK, result);

    dmGui::DeleteScene(scene);

    dmGui::DeleteScript(script);
}

void RenderNodesStoreTransform(dmGui::HScene scene, dmGui::HNode* nodes, const Vectormath::Aos::Matrix4* node_transforms,
        uint32_t node_count, void* context)
{
    Vectormath::Aos::Matrix4* out_transforms = (Vectormath::Aos::Matrix4*)context;
    memcpy(out_transforms, node_transforms, sizeof(Vectormath::Aos::Matrix4) * node_count);
}

TEST_F(dmGuiScriptTest, TestLocalTransformSetPos)
{
    dmGui::HScript script = NewScript(m_Context);

    dmGui::NewSceneParams params;
    params.m_MaxNodes = 64;
    params.m_MaxAnimations = 32;
    params.m_UserData = this;
    dmGui::HScene scene = dmGui::NewScene(m_Context, &params);
    dmGui::SetSceneScript(scene, script);

    // Set position
    const char* src =
            "function init(self)\n"
            "    local n1 = gui.new_box_node(vmath.vector3(1, 1, 1), vmath.vector3(1, 1, 1))\n"
            "    gui.set_pivot(n1, gui.PIVOT_SW)\n"
            "    gui.set_position(n1, vmath.vector3(2, 2, 2))\n"
            "end\n";

    dmGui::Result result = SetScript(script, src, strlen(src), "dummy_source");
    ASSERT_EQ(dmGui::RESULT_OK, result);

    result = dmGui::InitScene(scene);
    ASSERT_EQ(dmGui::RESULT_OK, result);

    Vectormath::Aos::Matrix4 transform;
    dmGui::RenderScene(scene, RenderNodesStoreTransform, &transform);

    ASSERT_NEAR(2.0f, transform.getElem(3, 0), EPSILON);
    ASSERT_NEAR(2.0f, transform.getElem(3, 1), EPSILON);
    ASSERT_NEAR(2.0f, transform.getElem(3, 2), EPSILON);

    dmGui::DeleteScene(scene);

    dmGui::DeleteScript(script);
}

TEST_F(dmGuiScriptTest, TestLocalTransformAnim)
{
    dmGui::HScript script = NewScript(m_Context);

    dmGui::NewSceneParams params;
    params.m_MaxNodes = 64;
    params.m_MaxAnimations = 32;
    params.m_UserData = this;
    dmGui::HScene scene = dmGui::NewScene(m_Context, &params);
    dmGui::SetSceneScript(scene, script);

    // Set position
    const char* src =
            "function init(self)\n"
            "    local n1 = gui.new_box_node(vmath.vector3(1, 1, 1), vmath.vector3(1, 1, 1))\n"
            "    gui.set_pivot(n1, gui.PIVOT_SW)\n"
            "    gui.set_position(n1, vmath.vector3(0, 0, 0))\n"
            "    gui.animate(n1, gui.PROP_POSITION, vmath.vector3(2, 2, 2), gui.EASING_LINEAR, 1)\n"
            "end\n";

    dmGui::Result result = SetScript(script, src, strlen(src), "dummy_source");
    ASSERT_EQ(dmGui::RESULT_OK, result);

    result = dmGui::InitScene(scene);
    ASSERT_EQ(dmGui::RESULT_OK, result);

    Vectormath::Aos::Matrix4 transform;
    dmGui::RenderScene(scene, RenderNodesStoreTransform, &transform);

    ASSERT_NEAR(0.0f, transform.getElem(3, 0), EPSILON);
    ASSERT_NEAR(0.0f, transform.getElem(3, 1), EPSILON);
    ASSERT_NEAR(0.0f, transform.getElem(3, 2), EPSILON);

    dmGui::UpdateScene(scene, 1.0f);

    dmGui::RenderScene(scene, RenderNodesStoreTransform, &transform);

    ASSERT_NEAR(2.0f, transform.getElem(3, 0), EPSILON);
    ASSERT_NEAR(2.0f, transform.getElem(3, 1), EPSILON);
    ASSERT_NEAR(2.0f, transform.getElem(3, 2), EPSILON);

    dmGui::DeleteScene(scene);

    dmGui::DeleteScript(script);
}

int main(int argc, char **argv)
{
    dmDDF::RegisterAllTypes();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
