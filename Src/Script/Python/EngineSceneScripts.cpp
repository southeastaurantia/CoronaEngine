#include "EngineScripts.h"


PyMethodDef EngineScripts::SceneScripts::PyScene_methods[] = {
    {"setDisplaySurface", (PyCFunction)PyScene_setDisplaySurface, METH_VARARGS, ""},
    {"setCamera", (PyCFunction)PyScene_setCamera, METH_VARARGS, ""},
    {"setSunDirection", (PyCFunction)PyScene_setSunDirection, METH_VARARGS, ""},
    {nullptr, nullptr, 0, nullptr}};

// ���Ͷ���
PyTypeObject EngineScripts::SceneScripts::PySceneType = {
    PyVarObject_HEAD_INIT(nullptr, 0) "CabbageEngine.Scene", // ������
    sizeof(PySceneObject),                                   // �����С
    0,                                                       // ������С
    (destructor)PyScene_dealloc,                             // ��������
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // ��־
    "This is a demo about binding c++",       // �ĵ�
    0, 0, 0, 0, 0, 0,                         // �����ֶ�
    PyScene_methods,                          // ������
    0, 0, 0, 0, 0, 0, 0,
    (initproc)PyScene_init, // __init__
    0,                      // __new__
    (newfunc)PyScene_new    // __new__
};

void EngineScripts::SceneScripts::PyScene_dealloc(PySceneObject *self)
{
#ifdef ENABLE_CABBAGE_FRAMEWORK
    delete self->cpp_obj;
    self->cpp_obj = nullptr;
#endif
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *EngineScripts::SceneScripts::PyScene_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PySceneObject *self;
    self = (PySceneObject *)type->tp_alloc(type, 0);
    if (self != nullptr)
    {
#ifdef ENABLE_CABBAGE_FRAMEWORK
        self->cpp_obj = nullptr;
#endif
    }
    return (PyObject *)self;
}

int EngineScripts::SceneScripts::PyScene_init(PySceneObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *winID_py_ptr = nullptr;
    PyObject *lightField_py_ptr = nullptr;
    static char *kwlist[] = {(char *)"winID", (char *)"lightField", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, &winID_py_ptr, &lightField_py_ptr))
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
        bool lightField = false;
        if (lightField_py_ptr)
        {
            lightField = PyObject_IsTrue(lightField_py_ptr);
        }
        void *surface = nullptr;
        if (winID_py_ptr)
        {
            surface = PyLong_AsVoidPtr(winID_py_ptr);
        }
        self->cpp_obj = new CabbageEngine::Scene(surface, lightField);
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

PyObject *EngineScripts::SceneScripts::PyScene_setCamera(PySceneObject *self, PyObject *args)
{
    PyObject *position, *forward, *worldup;
    float cameraFov;
    if (!PyArg_ParseTuple(args, "OOOf", &position, &forward, &worldup, &cameraFov))
    {
        return PyBool_FromLong(false);
    }

#ifdef ENABLE_CABBAGE_FRAMEWORK
    ktm::fvec3 cameraPosition, cameraForward, cameraWorldUp;
    {
        int SizeOfposition = PyList_Size(position);
        if (SizeOfposition == 3)
        {
            for (int i = 0; i < SizeOfposition; i++)
            {
                PyObject *Item = PyList_GetItem(position, i);
                PyArg_Parse(Item, "f", &cameraPosition[i]);
            }
        }

        int SizeOfforward = PyList_Size(forward);
        if (SizeOfforward == 3)
        {
            for (int i = 0; i < SizeOfforward; i++)
            {
                PyObject *Item = PyList_GetItem(forward, i);
                PyArg_Parse(Item, "f", &cameraForward[i]);
            }
        }

        int SizeOfworldup = PyList_Size(worldup);
        if (SizeOfworldup == 3)
        {
            for (int i = 0; i < SizeOfworldup; i++)
            {
                PyObject *Item = PyList_GetItem(worldup, i);
                PyArg_Parse(Item, "f", &cameraWorldUp[i]);
            }
        }
    }

    self->cpp_obj->setCamera(cameraPosition, cameraForward, cameraWorldUp, cameraFov);
#endif

    return PyBool_FromLong(true);
};

PyObject *EngineScripts::SceneScripts::PyScene_setDisplaySurface(PySceneObject *self, PyObject *args)
{
    PyObject *py_ptr;

    PyArg_ParseTuple(args, "O", &py_ptr);

#ifdef ENABLE_CABBAGE_FRAMEWORK
    self->cpp_obj->setDisplaySurface(PyLong_AsVoidPtr(py_ptr));
#endif

    return PyBool_FromLong(true);
}

PyObject *EngineScripts::SceneScripts::PyScene_setSunDirection(PySceneObject *self, PyObject *args)
{
    PyObject *direct;
    if (!PyArg_ParseTuple(args, "O", &direct))
    {
        return PyBool_FromLong(false);
    }

#ifdef ENABLE_CABBAGE_FRAMEWORK
    ktm::fvec3 sunDir;
    {
        int SizeOfposition = PyList_Size(direct);
        if (SizeOfposition == 3)
        {
            for (int i = 0; i < SizeOfposition; i++)
            {
                PyObject *Item = PyList_GetItem(direct, i);
                PyArg_Parse(Item, "f", &sunDir[i]);
            }
        }
    }

    self->cpp_obj->setSunDirection(sunDir);
#endif

    return PyBool_FromLong(true);
}