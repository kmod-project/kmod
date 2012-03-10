/*
 * Libkmod -- Python interface to kmod API.
 *
 * Copyright (C) 2012 Red Hat, Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Andy Grover <agrover redhat com>
 *
 */

#include <Python.h>
#include <libkmod.h>

typedef struct {
    PyObject_HEAD
    struct kmod_ctx *ctx;
} kmodobject;

static PyTypeObject LibKmodType;

static PyObject *LibKmodError;


/* ----------------------------------------------------------------------
 * kmod object initialization/deallocation
 */

static int
libkmod_init(kmodobject *self, PyObject *arg)
{
    char *mod_dir = NULL;

    if (!PyArg_ParseTuple(arg, "|s", &mod_dir))
       return -1;

    self->ctx = kmod_new(mod_dir, NULL);
    if (!self->ctx) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    return 0;
}

static void
libkmod_dealloc(kmodobject *self)
{
    /* if already closed, don't reclose it */
    if (self->ctx != NULL){
	    kmod_unref(self->ctx);
    }
    //self->ob_type->tp_free((PyObject*)self);
    PyObject_Del(self);
}

static PyObject *
liblvm_library_get_version(lvmobject *self)
{
    return Py_BuildValue("s", lvm_library_get_version());
}


static PyObject *
liblvm_close(lvmobject *self)
{
    /* if already closed, don't reclose it */
    if (self->libh != NULL)
        lvm_quit(self->libh);

    self->libh = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
liblvm_lvm_list_vg_names(lvmobject *self)
{
    struct dm_list *vgnames;
    const char *vgname;
    struct lvm_str_list *strl;
    PyObject * rv;
    int i = 0;

    if ((vgnames = lvm_list_vg_names(self->libh))== NULL)
    	goto error;

    if (dm_list_empty(vgnames))
    	goto error;

    rv = PyTuple_New(dm_list_size(vgnames));
    if (rv == NULL)
        return NULL;

    dm_list_iterate_items(strl, vgnames) {
    	vgname = strl->str;
        PyTuple_SET_ITEM(rv, i, PyString_FromString(vgname));
        i++;
    }

    return rv;

error:
	PyErr_SetObject(LibLVMError,liblvm_get_last_error(self));
	return NULL;
}

static PyObject *
liblvm_lvm_list_vg_uuids(lvmobject *self)
{
    struct dm_list *uuids;
    const char *uuid;
    struct lvm_str_list *strl;
    PyObject * rv;
    int i = 0;

    if ((uuids = lvm_list_vg_uuids(self->libh))== NULL)
    	goto error;

    if (dm_list_empty(uuids))
        goto error;

    rv = PyTuple_New(dm_list_size(uuids));
    if (rv == NULL)
        return NULL;
    dm_list_iterate_items(strl, uuids) {
    	uuid = strl->str;
        PyTuple_SET_ITEM(rv, i, PyString_FromString(uuid));
        i++;
    }

    return rv;

error:
    PyErr_SetObject(LibLVMError,liblvm_get_last_error(self));
   	return NULL;
}

static PyObject *
liblvm_lvm_vgname_from_pvid(lvmobject *self, PyObject *arg)
{
    const char *pvid;
    const char *vgname;
    if (!PyArg_ParseTuple(arg, "s", &pvid))
        return NULL;

    if((vgname = lvm_vgname_from_pvid(self->libh, pvid)) == NULL) {
    	PyErr_SetObject(LibLVMError,liblvm_get_last_error(self));
        return NULL;
    }

    return Py_BuildValue("s", vgname);
}

static PyObject *
liblvm_lvm_vgname_from_device(lvmobject *self, PyObject *arg)
{
    const char *device;
    const char *vgname;
    if (!PyArg_ParseTuple(arg, "s", &device))
        return NULL;

    if((vgname = lvm_vgname_from_device(self->libh, device)) == NULL) {
    	PyErr_SetObject(LibLVMError,liblvm_get_last_error(self));
        return NULL;
    }

    return Py_BuildValue("s", vgname);
}

static PyObject *
liblvm_lvm_config_reload(lvmobject *self)
{
	int rval;

    if((rval = lvm_config_reload(self->libh)) == -1) {
    	PyErr_SetObject(LibLVMError,liblvm_get_last_error(self));
    	return NULL;
    }

    return Py_BuildValue("i", rval);
}


static PyObject *
liblvm_lvm_scan(lvmobject *self)
{
	int rval;

    if((rval = lvm_scan(self->libh)) == -1) {
    	PyErr_SetObject(LibLVMError,liblvm_get_last_error(self));
    	return NULL;
    }

    return Py_BuildValue("i", rval);
}

static PyObject *
liblvm_lvm_config_override(lvmobject *self, PyObject *arg)
{
    const char *config;
    int rval;

    if (!PyArg_ParseTuple(arg, "s", &config))
       return NULL;

    if ((rval = lvm_config_override(self->libh, config)) == -1) {
    	PyErr_SetObject(LibLVMError,liblvm_get_last_error(self));
        return NULL;
    }
    return Py_BuildValue("i", rval);
}

/* ----------------------------------------------------------------------
 * Method tables and other bureaucracy
 */

static PyMethodDef Libkmod_methods[] = {
    /* LVM methods */
    { "getVersion",          (PyCFunction)liblvm_library_get_version, METH_NOARGS },
    { "vgOpen",                (PyCFunction)liblvm_lvm_vg_open, METH_VARARGS },
    { "vgCreate",                (PyCFunction)liblvm_lvm_vg_create, METH_VARARGS },
    { "close",                  (PyCFunction)liblvm_close, METH_NOARGS },
    { "configReload",          (PyCFunction)liblvm_lvm_config_reload, METH_NOARGS },
    { "configOverride",        (PyCFunction)liblvm_lvm_config_override, METH_VARARGS },
    { "scan",                   (PyCFunction)liblvm_lvm_scan, METH_NOARGS },
    { "listVgNames",          (PyCFunction)liblvm_lvm_list_vg_names, METH_NOARGS },
    { "listVgUuids",          (PyCFunction)liblvm_lvm_list_vg_uuids, METH_NOARGS },
    { "vgNameFromPvid",        (PyCFunction)liblvm_lvm_vgname_from_pvid, METH_VARARGS },
    { "vgNameFromDevice",        (PyCFunction)liblvm_lvm_vgname_from_device, METH_VARARGS },

    { NULL,             NULL}           /* sentinel */
};

static PyObject *
liblvm_vg_getattr(vgobject *self, char *name)
{
    return Py_FindMethod(liblvm_vg_methods, (PyObject *)self, name);
}

static PyObject *
liblvm_lv_getattr(vgobject *self, char *name)
{
    return Py_FindMethod(liblvm_lv_methods, (PyObject *)self, name);
}

static PyObject *
liblvm_pv_getattr(vgobject *self, char *name)
{
    return Py_FindMethod(liblvm_pv_methods, (PyObject *)self, name);
}

static PyTypeObject LibKmodType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "kmod.kmod",             /*tp_name*/
    sizeof(kmodobject),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)libkmod_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Libkmod objects",           /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    Libkmod_methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)libkmod_init,      /* tp_init */
    0,                         /* tp_alloc */
    0,
    //(newfunc)Libkmod_new,                 /* tp_new */
};

#ifndef PyMODINIT_FUNC    /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initlibkmod(void)
{
    PyObject *m;

    LibKmodType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&LibKmodType) < 0)
        return;

    m = Py_InitModule3("libkmod", Libkmod_methods, "Libkmod module");
    if (m == NULL)
        return;

    Py_INCREF(&LibKmodType);
    PyModule_AddObject(m, "LibKmod", (PyObject *)&LibKmodType);

    LibKmodError = PyErr_NewException("Libkmod.LibKmodError",
                                      NULL, NULL);
    if (LibKmodError) {
        /* Each call to PyModule_AddObject decrefs it; compensate: */
        Py_INCREF(LibKmodError);
        Py_INCREF(LibKmodError);
        PyModule_AddObject(m, "error", LibKmodError);
        PyModule_AddObject(m, "LibKmodError", LibKmodError);
    }

}

