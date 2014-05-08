#ifndef DM_GUI_PRIVATE_H
#define DM_GUI_PRIVATE_H

#include <dlib/index_pool.h>
#include <dlib/array.h>
#include <dlib/hashtable.h>
#include <dlib/easing.h>
#include <dlib/image.h>

#include "gui.h"

extern "C"
{
#include "lua/lua.h"
}

namespace dmGui
{
    const uint32_t MAX_MESSAGE_DATA_SIZE = 512;
    extern const uint16_t INVALID_INDEX;

    #define GUI_SCRIPT_INSTANCE "GuiScriptInstance"

    enum ScriptFunction
    {
        SCRIPT_FUNCTION_INIT,
        SCRIPT_FUNCTION_FINAL,
        SCRIPT_FUNCTION_UPDATE,
        SCRIPT_FUNCTION_ONMESSAGE,
        SCRIPT_FUNCTION_ONINPUT,
        SCRIPT_FUNCTION_ONRELOAD,
        MAX_SCRIPT_FUNCTION_COUNT
    };

    struct Context
    {
        lua_State*              m_LuaState;
        GetURLCallback          m_GetURLCallback;
        GetUserDataCallback     m_GetUserDataCallback;
        ResolvePathCallback     m_ResolvePathCallback;
        GetTextMetricsCallback  m_GetTextMetricsCallback;
        uint32_t                m_Width;
        uint32_t                m_Height;
        uint32_t                m_PhysicalWidth;
        uint32_t                m_PhysicalHeight;
        dmArray<HScene>         m_Scenes;
        dmArray<HNode>          m_RenderNodes;
        dmArray<Matrix4>        m_RenderTransforms;
        dmHID::HContext         m_HidContext;
        void*                   m_DefaultFont;
    };

    struct Node
    {
        Vector4     m_Properties[PROPERTY_COUNT];
        Vector4     m_ResetPointProperties[PROPERTY_COUNT];
        Matrix4     m_LocalTransform;
        uint32_t    m_ResetPointState;

        union
        {
            struct
            {
                uint32_t    m_BlendMode : 4;
                uint32_t    m_NodeType : 4;
                uint32_t    m_XAnchor : 2;
                uint32_t    m_YAnchor : 2;
                uint32_t    m_Pivot : 4;
                uint32_t    m_AdjustMode : 2;
                uint32_t    m_LineBreak : 1;
                uint32_t    m_Enabled : 1; // Only enabled (1) nodes are animated and rendered
                uint32_t    m_DirtyLocal : 1;
                uint32_t    m_Reserved : 11;
            };

            uint32_t m_State;
        };

        bool        m_HasResetPoint;
        const char* m_Text;
        uint64_t    m_TextureHash;
        void*       m_Texture;
        uint64_t    m_FontHash;
        void*       m_Font;
        dmhash_t    m_LayerHash;
        uint16_t    m_LayerIndex;
    };

    struct InternalNode
    {
        Node            m_Node;
        dmhash_t        m_NameHash;
        uint16_t        m_Version;
        uint16_t        m_Index;
        uint16_t        m_PrevIndex;
        uint16_t        m_NextIndex;
        uint16_t        m_ParentIndex;
        uint16_t        m_ChildHead;
        uint16_t        m_ChildTail;
        uint16_t        m_RenderKey;
        uint16_t        m_Deleted : 1; // Set to true for deferred deletion
        uint16_t        m_Padding : 15;
    };

    struct NodeProxy
    {
        HScene m_Scene;
        HNode  m_Node;
    };

    struct Animation
    {
        HNode    m_Node;
        float*   m_Value;
        float    m_From;
        float    m_To;
        float    m_Delay;
        float    m_Elapsed;
        float    m_Duration;
        dmEasing::Type m_Easing;
        Playback m_Playback;
        AnimationComplete m_AnimationComplete;
        void*    m_Userdata1;
        void*    m_Userdata2;
        uint16_t m_FirstUpdate : 1;
        uint16_t m_AnimationCompleteCalled : 1;
        uint16_t m_Cancelled : 1;
        uint16_t m_Backwards : 1;
    };

    struct Script
    {
        Script();

        int         m_FunctionReferences[MAX_SCRIPT_FUNCTION_COUNT];
        Context*    m_Context;
    };

    struct DynamicTexture
    {
        DynamicTexture(void* handle)
        {
            memset(this, 0, sizeof(*this));
            m_Handle = handle;
            m_Type = (dmImage::Type) -1;
        }
        void*           m_Handle;
        uint32_t        m_Created : 1;
        uint32_t        m_Deleted : 1;
        uint32_t        m_Width;
        uint32_t        m_Height;
        void*           m_Buffer;
        dmImage::Type   m_Type;
    };

    struct Scene
    {
        Scene();

        int                     m_InstanceReference;
        int                     m_DataReference;
        Context*                m_Context;
        Script*                 m_Script;
        dmIndexPool16           m_NodePool;
        dmArray<InternalNode>   m_Nodes;
        dmArray<Animation>      m_Animations;
        dmHashTable64<void*>    m_Textures;
        dmHashTable64<void*>    m_Fonts;
        dmHashTable64<DynamicTexture> m_DynamicTextures;
        dmHashTable64<uint16_t> m_Layers;
        dmArray<dmhash_t>       m_DeletedDynamicTextures;
        void*                   m_DefaultFont;
        void*                   m_UserData;
        uint16_t                m_RenderHead;
        uint16_t                m_RenderTail;
        uint16_t                m_NextVersionNumber;
        uint16_t                m_RenderOrder; // For the render-key
        uint16_t                m_NextLayerIndex;
        uint16_t                m_ResChanged : 1;
    };

    InternalNode* GetNode(HScene scene, HNode node);

    /** calculates the transform of a node
     * A boundary transform maps the local rectangle (0,1),(0,1) to screen space such that it inclusively encapsulates the node boundaries in screen space.
     * Box nodes are rendered in boundary space (quad with dimensions (0,1),(0,1)), so the same transform is calculated whether or not the boundary-flag is set.
     * Text nodes are rendered in a transform where the origin is located at the left edge of the base line, excluding the text size, since it's implicitly spanned by glyph quads.
     * Their boundary transform is analogue to the box boundary transform.
     * This is complicated and could be simplified by supporting different pivots when rendering text.
     *
     * @param scene scene of the node
     * @param node node for which to calculate the transform
     * @param reference_scale the reference scale of the scene which is the ratio between physical and reference dimensions
     * @param boundary true calculates the boundary transform, false calculates the render transform
     * @param include_size If the size should be included in the transform
     * @param reset_pivot If the pivot should be ignored in the resulting transform
     * @param out_transform out-parameter to write the calculated transform to
     */
    void CalculateNodeTransform(HScene scene, InternalNode* parent, const Vector4& reference_scale, bool boundary, bool include_size, bool reset_pivot, Matrix4* out_transform);

    /** calculates the reference scale for a context
     * The reference scale is defined as scaling from the predefined screen space to the actual screen space.
     *
     * @param context context for which to calculate the reference scale
     * @return a scaling vector (ref_scale, ref_scale, 1, 1)
     */
    Vector4 CalculateReferenceScale(HContext context);

    HNode GetNodeHandle(InternalNode* node);
    Vector4 CalculateReferenceScale(HContext context);
}

#endif
