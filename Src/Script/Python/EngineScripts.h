#pragma once

#include <Python.h>
#include <ktm/ktm.h>

#include "Core/CoronaEngineAPI.h"

struct EngineScripts
{
    struct SceneScripts
    {
        struct PySceneObject
        {
            PyObject_HEAD;
            CoronaEngine::CoronaEngineAPI::Scene *cpp_obj;
        };

        static void PyScene_dealloc(PySceneObject *self);
        static PyObject *PyScene_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
        static int PyScene_init(PySceneObject *self, PyObject *args, PyObject *kwds);

        static PyObject *PyScene_setCamera(PySceneObject *self, PyObject *args);
        static PyObject *PyScene_setDisplaySurface(PySceneObject *self, PyObject *args);
        static PyObject *PyScene_setSunDirection(PySceneObject *self, PyObject *args);

        static PyMethodDef PyScene_methods[];
        static PyTypeObject PySceneType;
    };

    struct ActorScripts
    {
        struct PyActorObject
        {
            PyObject_HEAD;
            CoronaEngine::CoronaEngineAPI::Actor *cpp_obj;
        };

        static void PyActor_dealloc(PyActorObject *self);
        static PyObject *PyActor_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
        static int PyActor_init(PyActorObject *self, PyObject *args, PyObject *kwds);

        static PyObject *PyActor_move(PyActorObject *self, PyObject *args);
        static PyObject *PyActor_rotate(PyActorObject *self, PyObject *args);
        static PyObject *PyActor_scale(PyActorObject *self, PyObject *args);

        static PyMethodDef PyActor_methods[];
        static PyTypeObject PyActorType;
    };
};