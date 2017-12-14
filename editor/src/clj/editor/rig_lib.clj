(ns editor.rig-lib
  (:require
   [dynamo.graph :as g]
   [editor.math :as math]
   [editor.protobuf :as protobuf]
   [editor.gl.vertex :as vertex]
   [editor.gl.shader :as shader])
  (:import
   #_[com.dynamo.rig.proto ...]
   (com.defold.libs RigLibrary RigLibrary$Result RigLibrary$NewContextParams)
   (com.sun.jna Memory Pointer)
   (com.sun.jna.ptr IntByReference)
   (com.jogamp.common.nio Buffers)
   (java.nio ByteBuffer)
   (javax.vecmath Point3d Quat4d Vector3d Matrix4d)
   (com.google.protobuf Message)))

(set! *warn-on-reflection* true)

(vertex/defvertex vertex-format
  (vec3 position)
  (vec4.ubyte color true)
  (vec2.ushort texcoord0 true))

(defn- create-context
  [max-rig-instance-count]
  (let [params (RigLibrary$NewContextParams. (int max-rig-instance-count))
        ret (RigLibrary/Rig_NewContext params)]
    (if (= ret RigLibrary$Result/RESULT_OK)
      (.getValue (.context params))
      (throw (Exception. "Rig_NewContext")))))

(defn- delete-context
  [context]
  (RigLibrary/Rig_DeleteContext context))


(comment

  (def c (create-context 4))

  (delete-context c)

  )
