#include"EngineScripts.h"

PyMethodDef EngineScripts::ActorScripts::PyActor_methods[] = {
    //{"destroy", (PyCFunction)PyActor_destroy, METH_VARARGS, ""},
    {"move", (PyCFunction)PyActor_move, METH_VARARGS, ""},
    {"rotate", (PyCFunction)PyActor_rotate, METH_VARARGS, ""},
    {"scale", (PyCFunction)PyActor_scale, METH_VARARGS, ""},
    {nullptr, nullptr, 0, nullptr}};

// ���Ͷ���
PyTypeObject EngineScripts::ActorScripts::PyActorType = {
    PyVarObject_HEAD_INIT(nullptr, 0) "CabbageEngine.Actor", // ������
    sizeof(PyActorObject),                                   // �����С
    0,                                                       // ������С
    (destructor)PyActor_dealloc,                             // ��������
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // ��־
    "This is a demo about binding c++",       // �ĵ�
    0, 0, 0, 0, 0, 0,                         // �����ֶ�
    PyActor_methods,                          // ������
    0, 0, 0, 0, 0, 0, 0,
    (initproc)PyActor_init, // __init__
    0,                      // __new__
    (newfunc)PyActor_new    // __new__
};


void EngineScripts::ActorScripts::PyActor_dealloc(PyActorObject *self)
{
#ifdef ENABLE_CABBAGE_FRAMEWORK
    CabbageEngine::pythonOperateList.destoryActor(self->cpp_obj->actorID);

    delete self->cpp_obj;
    self->cpp_obj = nullptr;
#endif

    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *EngineScripts::ActorScripts::PyActor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyActorObject *self;
    self = (PyActorObject *)type->tp_alloc(type, 0);
    if (self != nullptr)
    {
#ifdef ENABLE_CABBAGE_FRAMEWORK
        // EngineScripts::pythonOperateList.destoryActor(self->cpp_obj->actorID);
        self->cpp_obj = nullptr;
#endif
    }
    return (PyObject *)self;
}

int EngineScripts::ActorScripts::PyActor_init(PyActorObject *self, PyObject *args, PyObject *kwds)
{
    char *path = (char *)"";
    SceneScripts::PySceneObject *scene = nullptr;
    static char *kwlist[] = {(char *)"scene", (char *)"path", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Os", kwlist, &scene, &path))
    {
        return -1;
    }

#ifdef ENABLE_CABBAGE_FRAMEWORK
    if (self->cpp_obj)
    {
        delete self->cpp_obj;
        self->cpp_obj = nullptr;
    }

    try
    {
        self->cpp_obj = new CabbageEngine::Actor(*scene->cpp_obj, path);
    }
    catch (const std::bad_alloc &)
    {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate Actor C++ object");
        return -1;
    }
    catch (const std::exception &e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return -1;
    }
#endif

    return 0; // Success
}

PyObject *EngineScripts::ActorScripts::PyActor_move(PyActorObject *self, PyObject *args)
{
    PyObject *vector_py;

    if (!PyArg_ParseTuple(args, "O", &vector_py))
    {
        return nullptr;
    }

#ifdef ENABLE_CABBAGE_FRAMEWORK
    ktm::fvec3 vector_cpp;
    {
        PyObject *ItemX = PyList_GetItem(vector_py, 0);
        PyArg_Parse(ItemX, "f", &vector_cpp.x);

        PyObject *ItemY = PyList_GetItem(vector_py, 1);
        PyArg_Parse(ItemY, "f", &vector_cpp.y);

        PyObject *ItemZ = PyList_GetItem(vector_py, 2);
        PyArg_Parse(ItemZ, "f", &vector_cpp.z);
    }

    if (!self->cpp_obj)
    {
        PyErr_SetString(PyExc_ReferenceError, "Actor object not init.");
        return nullptr;
    }
    try
    {
        self->cpp_obj->move(vector_cpp);
    }
    catch (const std::exception &e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }
#endif

    Py_RETURN_NONE;
}

PyObject *EngineScripts::ActorScripts::PyActor_rotate(PyActorObject *self, PyObject *args)
{
    PyObject *vector_py;
    if (!PyArg_ParseTuple(args, "O", &vector_py))
    {
        return nullptr;
    }

#ifdef ENABLE_CABBAGE_FRAMEWORK
    ktm::fvec3 vector_cpp;
    {
        PyObject *ItemX = PyList_GetItem(vector_py, 0);
        PyArg_Parse(ItemX, "f", &vector_cpp.x);

        PyObject *ItemY = PyList_GetItem(vector_py, 1);
        PyArg_Parse(ItemY, "f", &vector_cpp.y);

        PyObject *ItemZ = PyList_GetItem(vector_py, 2);
        PyArg_Parse(ItemZ, "f", &vector_cpp.z);
    }

    if (!self->cpp_obj)
    {
        PyErr_SetString(PyExc_ReferenceError, "Actor object not init.");
        return nullptr;
    }
    try
    {
         self->cpp_obj->rotate(vector_cpp);
    }
    catch (const std::exception &e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }
#endif

    Py_RETURN_NONE;
}

PyObject *EngineScripts::ActorScripts::PyActor_scale(PyActorObject *self, PyObject *args)
{
    PyObject *vector_py;

    if (!PyArg_ParseTuple(args, "O", &vector_py))
    {
        return nullptr;
    }

#ifdef ENABLE_CABBAGE_FRAMEWORK
    ktm::fvec3 vector_cpp;
    {
        PyObject *ItemX = PyList_GetItem(vector_py, 0);
        PyArg_Parse(ItemX, "f", &vector_cpp.x);

        PyObject *ItemY = PyList_GetItem(vector_py, 1);
        PyArg_Parse(ItemY, "f", &vector_cpp.y);

        PyObject *ItemZ = PyList_GetItem(vector_py, 2);
        PyArg_Parse(ItemZ, "f", &vector_cpp.z);
    }

    if (!self->cpp_obj)
    {
        PyErr_SetString(PyExc_ReferenceError, "Actor object not init.");
        return nullptr;
    }
    try
    {
         self->cpp_obj->scale(vector_cpp);
    }
    catch (const std::exception &e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }
#endif

    Py_RETURN_NONE;
}