/*-----------------------------------------------------------------------------
| Copyright (c) 2013-2024, Nucleic Development Team.
|
| Distributed under the terms of the Modified BSD License.
|
| The full license is in the file LICENSE, distributed with this software.
|----------------------------------------------------------------------------*/
#include <cppy/cppy.h>
#include <kiwi/kiwi.h>
#include "types.h"
#include "util.h"


namespace kiwisolver
{

namespace
{

PyObject*
Solver_new( PyTypeObject* type, PyObject* args, PyObject* kwargs )
{
	if( PyTuple_GET_SIZE( args ) != 0 || ( kwargs && PyDict_Size( kwargs ) != 0 ) )
		return cppy::type_error( "Solver.__new__ takes no arguments" );
	PyObject* pysolver = PyType_GenericNew( type, args, kwargs );
	if( !pysolver )
		return 0;
	Solver* self = reinterpret_cast<Solver*>( pysolver );
	ACQUIRE_GLOBAL_LOCK();
	new( &self->solver ) kiwi::Solver();
	RELEASE_GLOBAL_LOCK();
	return pysolver;
}


void
Solver_dealloc( Solver* self )
{
	ACQUIRE_GLOBAL_LOCK();
	self->solver.~Solver();
	RELEASE_GLOBAL_LOCK();
	Py_TYPE( self )->tp_free( pyobject_cast( self ) );
}


PyObject*
Solver_addConstraint( Solver* self, PyObject* other )
{
	if( !Constraint::TypeCheck( other ) )
		return cppy::type_error( other, "Constraint" );
	Constraint* cn = reinterpret_cast<Constraint*>( other );
	try
	{
		ACQUIRE_GLOBAL_LOCK();
		self->solver.addConstraint( cn->constraint );
		RELEASE_GLOBAL_LOCK();
	}
	catch( const kiwi::DuplicateConstraint& )
	{
		RELEASE_GLOBAL_LOCK();
		PyErr_SetObject( DuplicateConstraint, other );
		return 0;
	}
	catch( const kiwi::UnsatisfiableConstraint& )
	{
		RELEASE_GLOBAL_LOCK();
		PyErr_SetObject( UnsatisfiableConstraint, other );
		return 0;
	}
	Py_RETURN_NONE;
}


PyObject*
Solver_removeConstraint( Solver* self, PyObject* other )
{
	if( !Constraint::TypeCheck( other ) )
		return cppy::type_error( other, "Constraint" );
	Constraint* cn = reinterpret_cast<Constraint*>( other );
	try
	{
		ACQUIRE_GLOBAL_LOCK();
		self->solver.removeConstraint( cn->constraint );
		RELEASE_GLOBAL_LOCK();
	}
	catch( const kiwi::UnknownConstraint& )
	{
		RELEASE_GLOBAL_LOCK();
		PyErr_SetObject( UnknownConstraint, other );
		return 0;
	}
	Py_RETURN_NONE;
}


PyObject*
Solver_hasConstraint( Solver* self, PyObject* other )
{
	if( !Constraint::TypeCheck( other ) )
		return cppy::type_error( other, "Constraint" );
	Constraint* cn = reinterpret_cast<Constraint*>( other );

	ACQUIRE_GLOBAL_LOCK();
	bool hasConstraint = self->solver.hasConstraint( cn->constraint );
	RELEASE_GLOBAL_LOCK();

	return cppy::incref( hasConstraint ? Py_True : Py_False );
}


PyObject*
Solver_addEditVariable( Solver* self, PyObject* args )
{
	PyObject* pyvar;
	PyObject* pystrength;
	if( !PyArg_ParseTuple( args, "OO", &pyvar, &pystrength ) )
		return 0;
	if( !Variable::TypeCheck( pyvar ) )
		return cppy::type_error( pyvar, "Variable" );
	double strength;
	if( !convert_to_strength( pystrength, strength ) )
		return 0;
	Variable* var = reinterpret_cast<Variable*>( pyvar );
	try
	{
		ACQUIRE_GLOBAL_LOCK();
		self->solver.addEditVariable( var->variable, strength );
		RELEASE_GLOBAL_LOCK();
	}
	catch( const kiwi::DuplicateEditVariable& )
	{
		RELEASE_GLOBAL_LOCK();
		PyErr_SetObject( DuplicateEditVariable, pyvar );
		return 0;
	}
	catch( const kiwi::BadRequiredStrength& e )
	{
		RELEASE_GLOBAL_LOCK();
		PyErr_SetString( BadRequiredStrength, e.what() );
		return 0;
	}
	Py_RETURN_NONE;
}


PyObject*
Solver_removeEditVariable( Solver* self, PyObject* other )
{
	if( !Variable::TypeCheck( other ) )
		return cppy::type_error( other, "Variable" );
	Variable* var = reinterpret_cast<Variable*>( other );
	try
	{
		ACQUIRE_GLOBAL_LOCK();
		self->solver.removeEditVariable( var->variable );
		RELEASE_GLOBAL_LOCK();
	}
	catch( const kiwi::UnknownEditVariable& )
	{
		RELEASE_GLOBAL_LOCK();
		PyErr_SetObject( UnknownEditVariable, other );
		return 0;
	}
	Py_RETURN_NONE;
}


PyObject*
Solver_hasEditVariable( Solver* self, PyObject* other )
{
	if( !Variable::TypeCheck( other ) )
		return cppy::type_error( other, "Variable" );
	Variable* var = reinterpret_cast<Variable*>( other );
	ACQUIRE_GLOBAL_LOCK();
	bool hasEditVariable = self->solver.hasEditVariable( var->variable );
	RELEASE_GLOBAL_LOCK();
	return cppy::incref( hasEditVariable ? Py_True : Py_False );
}


PyObject*
Solver_suggestValue( Solver* self, PyObject* args )
{
	PyObject* pyvar;
	PyObject* pyvalue;
	if( !PyArg_ParseTuple( args, "OO", &pyvar, &pyvalue ) )
		return 0;
	if( !Variable::TypeCheck( pyvar ) )
		return cppy::type_error( pyvar, "Variable" );
	double value;
	if( !convert_to_double( pyvalue, value ) )
		return 0;
	Variable* var = reinterpret_cast<Variable*>( pyvar );
	try
	{
		ACQUIRE_GLOBAL_LOCK();
		self->solver.suggestValue( var->variable, value );
		RELEASE_GLOBAL_LOCK();
	}
	catch( const kiwi::UnknownEditVariable& )
	{
		RELEASE_GLOBAL_LOCK();
		PyErr_SetObject( UnknownEditVariable, pyvar );
		return 0;
	}
	Py_RETURN_NONE;
}


PyObject*
Solver_updateVariables( Solver* self )
{
	ACQUIRE_GLOBAL_LOCK();
	self->solver.updateVariables();
	RELEASE_GLOBAL_LOCK();
	Py_RETURN_NONE;
}


PyObject*
Solver_reset( Solver* self )
{
	ACQUIRE_GLOBAL_LOCK();
	self->solver.reset();
	RELEASE_GLOBAL_LOCK();
	Py_RETURN_NONE;
}


PyObject*
Solver_dump( Solver* self )
{
	ACQUIRE_GLOBAL_LOCK();
	std::string dumps = self->solver.dumps();
	RELEASE_GLOBAL_LOCK();
	cppy::ptr dump_str( PyUnicode_FromString( dumps.c_str() ) );
	PyObject_Print( dump_str.get(), stdout, 0 );
	Py_RETURN_NONE;
}

PyObject*
Solver_dumps( Solver* self )
{
	ACQUIRE_GLOBAL_LOCK();
	std::string dumps = self->solver.dumps();
	RELEASE_GLOBAL_LOCK();
	return PyUnicode_FromString( dumps.c_str() );
}

static PyMethodDef
Solver_methods[] = {
	{ "addConstraint", ( PyCFunction )Solver_addConstraint, METH_O,
	  "Add a constraint to the solver." },
	{ "removeConstraint", ( PyCFunction )Solver_removeConstraint, METH_O,
	  "Remove a constraint from the solver." },
	{ "hasConstraint", ( PyCFunction )Solver_hasConstraint, METH_O,
	  "Check whether the solver contains a constraint." },
	{ "addEditVariable", ( PyCFunction )Solver_addEditVariable, METH_VARARGS,
	  "Add an edit variable to the solver." },
	{ "removeEditVariable", ( PyCFunction )Solver_removeEditVariable, METH_O,
	  "Remove an edit variable from the solver." },
	{ "hasEditVariable", ( PyCFunction )Solver_hasEditVariable, METH_O,
	  "Check whether the solver contains an edit variable." },
	{ "suggestValue", ( PyCFunction )Solver_suggestValue, METH_VARARGS,
	  "Suggest a desired value for an edit variable." },
	{ "updateVariables", ( PyCFunction )Solver_updateVariables, METH_NOARGS,
	  "Update the values of the solver variables." },
	{ "reset", ( PyCFunction )Solver_reset, METH_NOARGS,
	  "Reset the solver to the initial empty starting condition." },
	{ "dump", ( PyCFunction )Solver_dump, METH_NOARGS,
	  "Dump a representation of the solver internals to stdout." },
	{ "dumps", ( PyCFunction )Solver_dumps, METH_NOARGS,
	  "Dump a representation of the solver internals to a string." },
	{ 0 } // sentinel
};


static PyType_Slot Solver_Type_slots[] = {
    { Py_tp_dealloc, void_cast( Solver_dealloc ) },      /* tp_dealloc */
    { Py_tp_methods, void_cast( Solver_methods ) },      /* tp_methods */
    { Py_tp_new, void_cast( Solver_new ) },              /* tp_new */
    { Py_tp_alloc, void_cast( PyType_GenericAlloc ) },   /* tp_alloc */
    { Py_tp_free, void_cast( PyObject_Del ) },           /* tp_free */
    { 0, 0 },
};


} // namespace


// Initialize static variables (otherwise the compiler eliminates them)
PyTypeObject* Solver::TypeObject = NULL;


PyType_Spec Solver::TypeObject_Spec = {
	"kiwisolver.Solver",             /* tp_name */
	sizeof( Solver ),                /* tp_basicsize */
	0,                                   /* tp_itemsize */
	Py_TPFLAGS_DEFAULT|
    Py_TPFLAGS_BASETYPE,                 /* tp_flags */
    Solver_Type_slots                /* slots */
};


bool Solver::Ready()
{
    // The reference will be handled by the module to which we will add the type
	TypeObject = pytype_cast( PyType_FromSpec( &TypeObject_Spec ) );
    if( !TypeObject )
    {
        return false;
    }
    return true;
}


PyObject* DuplicateConstraint;

PyObject* UnsatisfiableConstraint;

PyObject* UnknownConstraint;

PyObject* DuplicateEditVariable;

PyObject* UnknownEditVariable;

PyObject* BadRequiredStrength;


bool init_exceptions()
{
	cppy::ptr mod( PyImport_ImportModule( "kiwisolver.exceptions" ) );
    if( !mod )
    {
        return false;
    }

 	DuplicateConstraint = mod.getattr( "DuplicateConstraint" );
 	if( !DuplicateConstraint )
    {
        return false;
    }

  	UnsatisfiableConstraint = mod.getattr( "UnsatisfiableConstraint" );
 	if( !UnsatisfiableConstraint )
 	{
        return false;
    }

  	UnknownConstraint = mod.getattr( "UnknownConstraint" );
 	if( !UnknownConstraint )
 	{
        return false;
    }

  	DuplicateEditVariable = mod.getattr( "DuplicateEditVariable" );
 	if( !DuplicateEditVariable )
 	{
        return false;
    }

  	UnknownEditVariable = mod.getattr( "UnknownEditVariable" );
 	if( !UnknownEditVariable )
 	{
        return false;
    }

  	BadRequiredStrength = mod.getattr( "BadRequiredStrength" );
 	if( !BadRequiredStrength )
 	{
        return false;
    }

	return true;
}

}  // namespace
