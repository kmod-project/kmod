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
libkmod_library_get_version(kmodobject *self)
{
    return Py_BuildValue("s", "whatup dude v1.234");
}

/* ----------------------------------------------------------------------
 * Method tables and other bureaucracy
 */

static PyMethodDef Libkmod_methods[] = {
    /* LVM methods */
    { "getVersion",          (PyCFunction)libkmod_library_get_version, METH_NOARGS },
    { NULL,             NULL}           /* sentinel */
};

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

