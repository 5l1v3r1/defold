package com.dynamo.cr.ddfeditor.scene;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;

import com.dynamo.cr.go.core.ComponentTypeNode;
import com.dynamo.cr.properties.Property;
import com.dynamo.cr.properties.Property.EditorType;
import com.dynamo.cr.properties.Resource;
import com.dynamo.cr.sceneed.core.AABB;
import com.dynamo.cr.sceneed.core.ISceneModel;
import com.dynamo.cr.sceneed.core.Node;
import com.dynamo.physics.proto.Physics.CollisionObjectType;

@SuppressWarnings("serial")
public class CollisionObjectNode extends ComponentTypeNode {

    @Property(editorType=EditorType.RESOURCE, extensions={"convexshape", "tilegrid", "tilemap"})
    @Resource
    private String collisionShape = "";
    @Property private CollisionObjectType type = CollisionObjectType.COLLISION_OBJECT_TYPE_DYNAMIC;
    @Property private float mass;
    @Property private float friction;
    @Property private float restitution;
    @Property private float linearDamping;
    @Property private float angularDamping;
    @Property private boolean lockedRotation;
    @Property private String group = "";
    @Property private String mask = "";
    private Node collisionShapeNode;

    protected IStatus validateCollisionShape() {
        if (getChildren().size() == 0 && collisionShape.length() == 0) {
            return new Status(IStatus.INFO, "com.dynamo", "Add shapes or a collision file.");
        } else {
            return Status.OK_STATUS;
        }
    }

    @Override
    public boolean handleReload(IFile file, boolean childWasReloaded) {
        if (!this.collisionShape.isEmpty()) {
            IFile shapeFile = getModel().getFile(this.collisionShape);
            if (shapeFile.exists() && shapeFile.equals(file)) {
                if (reloadCollisionShape()) {
                    return true;
                }
            }
        }
        return false;
    }

    private boolean reloadCollisionShape() {
        ISceneModel model = getModel();
        if (model != null) {
            try {
                this.collisionShapeNode = model.loadNode(this.collisionShape);
                updateAABB();
                return true;
            } catch (Throwable e) {
                // no reason to handle exception since having a null collision shape is invalid state, will be caught in resource validation
                this.collisionShapeNode = null;
            }
        }
        return false;
    }

    public String getCollisionShape() {
        return this.collisionShape;
    }

    public void setCollisionShape(String collisionShape) {
        this.collisionShape = collisionShape;
    }

    public CollisionObjectType getType() {
        return this.type;
    }

    public void setType(CollisionObjectType type) {
        this.type = type;
    }

    public float getMass() {
        if (isMassEditable())
            return this.mass;
        else
            return 0.0f;
    }

    public void setMass(float mass) {
        this.mass = mass;
    }

    public boolean isMassEditable() {
        return getType() == CollisionObjectType.COLLISION_OBJECT_TYPE_DYNAMIC;
    }

    public float getFriction() {
        return this.friction;
    }

    public void setFriction(float friction) {
        this.friction = friction;
    }

    public float getRestitution() {
        return this.restitution;
    }

    public void setRestitution(float restitution) {
        this.restitution = restitution;
    }

    public float getLinearDamping() {
        return this.linearDamping;
    }

    public void setLinearDamping(float linearDamping) {
        this.linearDamping = linearDamping;
    }

    public float getAngularDamping() {
        return this.angularDamping;
    }

    public void setAngularDamping(float angularDamping) {
        this.angularDamping = angularDamping;
    }

    public boolean getLockedRotation() {
        return this.lockedRotation;
    }

    public void setLockedRotation(boolean lockedRotation) {
        this.lockedRotation = lockedRotation;
    }

    public String getGroup() {
        return group;
    }

    public void setGroup(String group) {
        this.group = group;
    }

    public String getMask() {
        return mask;
    }

    public void setMask(String mask) {
        this.mask = mask;
    }

    public void setCollisionShapeNode(Node collisionShapeNode) {
        this.collisionShapeNode = collisionShapeNode;
        updateAABB();
    }

    public Node getCollisionShapeNode() {
        return collisionShapeNode;
    }

    private final void updateAABB() {
        AABB aabb = new AABB();
        if (this.collisionShapeNode != null) {
            this.collisionShapeNode.getAABB(aabb);
        }
        setAABB(aabb);
    }

}
