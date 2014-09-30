package com.dynamo.bob.fs.test;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertEquals;

import java.util.ArrayList;
import java.util.Collection;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.dynamo.bob.fs.FileSystemWalker;
import com.dynamo.bob.fs.IResource;
import com.dynamo.bob.fs.ZipMountPoint;

public class ZipMountPointTest {

    ZipMountPoint mp;

    @Before
    public void setUp() throws Exception {
        this.mp = new ZipMountPoint(null, "server_root/test_lib.zip");
        this.mp.mount();
    }

    @After
    public void tearDown() throws Exception {
        this.mp.unmount();
    }

    @Test
    public void testResource() throws Exception {
        IResource resource = mp.get("test_lib/file1.in");
        assertTrue(resource != null);
        assertTrue(!new String(resource.getContent()).equals("file1"));
    }

    @Test
    public void testMount() throws Exception {
        ZipMountPoint mp = new ZipMountPoint(null, "server_root/test_lib2.zip");
        assertTrue(mp.get("test_lib2/file2.in") == null);
        mp.mount();
        assertTrue(mp.get("test_lib2/file2.in") != null);
        mp.unmount();
        assertTrue(mp.get("test_lib2/file2.in") == null);
    }

    @Test
    public void testWalker() throws Exception {
        FileSystemWalker walker = new FileSystemWalker();
        Collection<String> results = new ArrayList<String>();
        this.mp.walk(".", walker, results);
        assertEquals(1, results.size());
        assertTrue(results.contains("test_lib/file1.in"));
    }
}

