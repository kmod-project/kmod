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

static PyTypeObject KmodType;

static PyObject *LibKmodError;


/* ----------------------------------------------------------------------
 * kmod object initialization/deallocation
 */

static int
libkmod_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    kmodobject *kmod = (kmodobject *)self;
    char *mod_dir = NULL;

    if (!PyArg_ParseTuple(args, "|s", &mod_dir))
       return -1;

    kmod->ctx = kmod_new(mod_dir, NULL);
    if (!kmod->ctx) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    return 0;
}

static void
libkmod_dealloc(PyObject *self)
{
    kmodobject *kmod = (kmodobject *)self;

    /* if already closed, don't reclose it */
    if (kmod->ctx != NULL){
	    kmod_unref(kmod->ctx);
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


static PyTypeObject KmodType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name = "kmod.kmod",
    .tp_basicsize = sizeof(kmodobject),
    //.tp_new = PyType_GenericNew,
    .tp_init = libkmod_init,
    .tp_dealloc = libkmod_dealloc,
    .tp_flags =Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "Libkmod objects",
    .tp_methods = Libkmod_methods,
};

#ifndef PyMODINIT_FUNC    /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initlibkmod(void)
{
    PyObject *m;

    KmodType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&KmodType) < 0)
        return;

    m = Py_InitModule3("libkmod", Libkmod_methods, "Libkmod module");
    if (m == NULL)
        return;

    Py_INCREF(&KmodType);
    PyModule_AddObject(m, "Kmod", (PyObject *)&KmodType);

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

