package com.dynamo.cr.guieditor.operations;

import java.util.Arrays;

import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.commands.operations.AbstractOperation;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;

import com.dynamo.cr.guieditor.scene.EditorFontDesc;
import com.dynamo.cr.guieditor.scene.GuiScene;

public class AddFontOperation extends AbstractOperation {

    private GuiScene scene;
    private String name;
    private String font;
    private EditorFontDesc fontDesc;

    public AddFontOperation(GuiScene scene, String name, String font) {
        super("Add Font");
        this.scene = scene;
        this.name = name;
        this.font = font;
    }

    @Override
    public IStatus execute(IProgressMonitor monitor, IAdaptable info)
            throws ExecutionException {
        fontDesc = scene.addFont(name, font);
        return Status.OK_STATUS;
    }

    @Override
    public IStatus redo(IProgressMonitor monitor, IAdaptable info)
            throws ExecutionException {
        fontDesc = scene.addFont(name, font);
        return Status.OK_STATUS;
    }

    @Override
    public IStatus undo(IProgressMonitor monitor, IAdaptable info)
            throws ExecutionException {
        scene.removeFonts(Arrays.asList(fontDesc));
        return Status.OK_STATUS;
    }

}