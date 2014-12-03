#ifndef __INCLUDED_SINGLETON_H_
#define __INCLUDED_SINGLETON_H_

#define DECLARE_SINGLETON_CLASS(C) \
private:\
	static C *m_pInstance;\
protected:\
	C() {;}\
	virtual ~C() {;}\
public:\
	static C *GetInstance()\
{\
	if (m_pInstance == NULL)\
	{\
	m_pInstance = new C;\
	}\
	return m_pInstance;\
}\
	static void DestoryInstance()\
{\
	if (m_pInstance != NULL)\
	{\
	delete m_pInstance;\
	m_pInstance = NULL;\
	}\
}


#define IMPLEMENT_SINGLETON_INSTANCE(C) C *C::m_pInstance = NULL;

#endif