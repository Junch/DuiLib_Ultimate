﻿#include "StdAfx.h"
#include "UIDateTime.h"

namespace DuiLib
{
	//CDateTimeUI::m_nDTUpdateFlag
#define DT_NONE   0
#define DT_UPDATE 1
#define DT_DELETE 2
#define DT_KEEP   3

	class CDateTimeWnd : public CWindowWnd
	{
	public:
		CDateTimeWnd();

		void Init(CDateTimeUI* pOwner);
		RECT CalPos();

		LPCTSTR GetWindowClassName() const;
		LPCTSTR GetSuperClassName() const;
		void OnFinalMessage(HWND hWnd);

		LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	protected:
		CDateTimeUI* m_pOwner;
		HBRUSH m_hBkBrush;
		bool m_bInit;
		bool m_bDropOpen;
		SYSTEMTIME m_oldSysTime;
	};

	CDateTimeWnd::CDateTimeWnd() : m_pOwner(NULL), m_hBkBrush(NULL), m_bInit(false), m_bDropOpen(false)
	{
	}

	void CDateTimeWnd::Init(CDateTimeUI* pOwner)
	{
		m_pOwner = pOwner;
		m_pOwner->m_nDTUpdateFlag = DT_NONE;

		if (m_hWnd == NULL)
		{
			RECT rcPos = CalPos();
			UINT uStyle = WS_CHILD;
			Create(m_pOwner->GetManager()->GetPaintWindow(), NULL, uStyle, 0, rcPos);
			SetWindowFont(m_hWnd, m_pOwner->GetManager()->GetFontInfo(m_pOwner->GetFont())->hFont, TRUE);
		}
		// 使用系统当前时间
		if (m_pOwner->GetText().IsEmpty()) {
			::GetLocalTime(&m_pOwner->m_sysTime);
		}
		// 显示格式
		CDuiString sTimeFormat = m_pOwner->GetTimeFormat();
		::SendMessage(m_hWnd, DTM_SETFORMAT, 0, LPARAM(sTimeFormat.GetData()));
		memcpy(&m_oldSysTime, &m_pOwner->m_sysTime, sizeof(SYSTEMTIME));
		::SendMessage(m_hWnd, DTM_SETSYSTEMTIME, 0, (LPARAM)&m_pOwner->m_sysTime);
		::ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
		::SetFocus(m_hWnd);

		m_bInit = true;    
	}

	RECT CDateTimeWnd::CalPos()
	{
		CDuiRect rcPos = m_pOwner->GetPos();
		//CDuiRect rcPadding = m_pOwner->GetTextPadding();
		//rcPos.left += rcPadding.left;
		//rcPos.top += rcPadding.top;
		//rcPos.right -= rcPadding.right;
		//rcPos.bottom -= rcPadding.bottom;

		CControlUI* pParent = m_pOwner;
		RECT rcParent;
		while( pParent = pParent->GetParent() ) {
			if( !pParent->IsVisible() ) {
				rcPos.left = rcPos.top = rcPos.right = rcPos.bottom = 0;
				break;
			}
			rcParent = pParent->GetClientPos();
			if( !::IntersectRect(&rcPos, &rcPos, &rcParent) ) {
				rcPos.left = rcPos.top = rcPos.right = rcPos.bottom = 0;
				break;
			}
		}

		return rcPos;
	}

	LPCTSTR CDateTimeWnd::GetWindowClassName() const
	{
		return _T("DateTimeWnd");
	}

	LPCTSTR CDateTimeWnd::GetSuperClassName() const
	{
		return DATETIMEPICK_CLASS;
	}

	void CDateTimeWnd::OnFinalMessage(HWND hWnd)
	{
		if( m_hBkBrush != NULL ) ::DeleteObject(m_hBkBrush);
		//m_pOwner->GetManager()->RemoveNativeWindow(hWnd);
		m_pOwner->m_pWindow = NULL;
		delete this;
	}

	LRESULT CDateTimeWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lRes = 0;
		BOOL bHandled = TRUE;
		if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
			memcpy(&m_pOwner->m_sysTime, &m_oldSysTime, sizeof(SYSTEMTIME));
			m_pOwner->m_nDTUpdateFlag = DT_UPDATE;
			m_pOwner->UpdateText();
			PostMessage(WM_CLOSE);
			return lRes;
		}
		else if(uMsg == OCM_NOTIFY) {
			NMHDR* pHeader=(NMHDR*)lParam;
			if(pHeader != NULL && pHeader->hwndFrom == m_hWnd) {
				if(pHeader->code == DTN_DATETIMECHANGE) {
					LPNMDATETIMECHANGE lpChage=(LPNMDATETIMECHANGE)lParam;
					::SendMessage(m_hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&m_pOwner->m_sysTime);
					m_pOwner->m_nDTUpdateFlag = DT_UPDATE;
					m_pOwner->UpdateText();
				}
				else if(pHeader->code == DTN_DROPDOWN) {
					m_bDropOpen = true;
				}
				else if(pHeader->code == DTN_CLOSEUP) {
					::SendMessage(m_hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&m_pOwner->m_sysTime);
					m_pOwner->m_nDTUpdateFlag = DT_UPDATE;
					m_pOwner->UpdateText();
					PostMessage(WM_CLOSE);
					m_bDropOpen = false;
				}
			}
			bHandled = FALSE;
		}
		else if(uMsg == WM_KILLFOCUS)
		{
			if(!m_bDropOpen) {
				PostMessage(WM_CLOSE);
			}
			bHandled = FALSE;
		}
		else bHandled = FALSE;
		if( !bHandled ) return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
		return lRes;
	}
	//////////////////////////////////////////////////////////////////////////
	//
	IMPLEMENT_DUICONTROL(CDateTimeUI)

	CDateTimeUI::CDateTimeUI()
	{
		::GetLocalTime(&m_sysTime);
		m_bReadOnly = false;
		m_sTimeFormat = _T("yyyy-MM-dd HH:mm:ss");
		m_pWindow = NULL;

		m_nDTUpdateFlag=DT_UPDATE;
		UpdateText();
		m_nDTUpdateFlag = DT_NONE;
	}

	LPCTSTR CDateTimeUI::GetClass() const
	{
		return _T("DateTimeUI");
	}

	LPVOID CDateTimeUI::GetInterface(LPCTSTR pstrName)
	{
		if( _tcscmp(pstrName, DUI_CTR_DATETIME) == 0 ) return static_cast<CDateTimeUI*>(this);
		return CLabelUI::GetInterface(pstrName);
	}

	SYSTEMTIME& CDateTimeUI::GetTime()
	{
		return m_sysTime;
	}

	void CDateTimeUI::SetTime(SYSTEMTIME* pst)
	{
		m_sysTime = *pst;
		Invalidate();
		m_nDTUpdateFlag = DT_UPDATE;
		UpdateText();
		m_nDTUpdateFlag = DT_NONE;
	}

	void CDateTimeUI::SetReadOny(bool bReadOnly)
	{
		m_bReadOnly = bReadOnly;
		Invalidate();
	}

	bool CDateTimeUI::IsReadOnly() const
	{
		return m_bReadOnly;
	}

	void CDateTimeUI::SetTimeFormat(LPCTSTR pstrFormat)
	{
		m_sTimeFormat = pstrFormat;
		Invalidate();

		m_nDTUpdateFlag = DT_UPDATE;
		UpdateText();
		m_nDTUpdateFlag = DT_NONE;
	}

	CDuiString CDateTimeUI::GetTimeFormat() const
	{
		return m_sTimeFormat;
	}

	void CDateTimeUI::UpdateText()
	{
		if (m_nDTUpdateFlag == DT_DELETE) {
			SetText(_T(""));
		}
		else if (m_nDTUpdateFlag == DT_UPDATE) {
			CDuiString sText;
			if(m_sTimeFormat.CompareNoCase(_T("yyyy-MM-dd")) == 0) {
				sText.SmallFormat(_T("%4d-%02d-%02d"), m_sysTime.wYear, m_sysTime.wMonth, m_sysTime.wDay);
			}
			else if(m_sTimeFormat.CompareNoCase(_T("HH:mm:ss")) == 0) {
				sText.SmallFormat(_T("%02d:%02d:%02d"),  m_sysTime.wHour, m_sysTime.wMinute, m_sysTime.wSecond);
			}
			else {
				sText.SmallFormat(_T("%4d-%02d-%02d %02d:%02d:%02d"), m_sysTime.wYear, m_sysTime.wMonth, m_sysTime.wDay, m_sysTime.wHour, m_sysTime.wMinute, m_sysTime.wSecond);
			}
			SetText(sText);
		}
	}

	void CDateTimeUI::DoEvent(TEventUI& event)
	{
		if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
			if( m_pParent != NULL ) m_pParent->DoEvent(event);
			else CLabelUI::DoEvent(event);
			return;
		}

		if( event.Type == UIEVENT_WINDOWSIZE )
		{
			if( m_pWindow != NULL ) m_pManager->SetFocusNeeded(this);
		}
		if( event.Type == UIEVENT_SCROLLWHEEL )
		{
			if( m_pWindow != NULL ) return;
		}
		if( event.Type == UIEVENT_SETFOCUS && IsEnabled() ) 
		{
			if( m_pWindow ) return;
			m_pWindow = new CDateTimeWnd();
			ASSERT(m_pWindow);
			m_pWindow->Init(this);
			m_pWindow->ShowWindow();
		}
		if( event.Type == UIEVENT_KILLFOCUS && IsEnabled() ) 
		{
			Invalidate();
		}
		if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK || event.Type == UIEVENT_RBUTTONDOWN) 
		{
			if( IsEnabled() ) {
				GetManager()->ReleaseCapture();
				if( IsFocused() && m_pWindow == NULL )
				{
					m_pWindow = new CDateTimeWnd();
					ASSERT(m_pWindow);
				}
				if( m_pWindow != NULL )
				{
					m_pWindow->Init(this);
					m_pWindow->ShowWindow();
				}
			}
			return;
		}
		if( event.Type == UIEVENT_MOUSEMOVE ) 
		{
			return;
		}
		if( event.Type == UIEVENT_BUTTONUP ) 
		{
			return;
		}
		if( event.Type == UIEVENT_CONTEXTMENU )
		{
			return;
		}
		if( event.Type == UIEVENT_MOUSEENTER )
		{
			return;
		}
		if( event.Type == UIEVENT_MOUSELEAVE )
		{
			return;
		}

		CLabelUI::DoEvent(event);
	}

	void CDateTimeUI::SetPos(RECT rc, bool bNeedInvalidate)
	{
		CControlUI::SetPos(rc, bNeedInvalidate);
		if( m_pWindow != NULL ) {
			RECT rcPos = m_pWindow->CalPos();
			::SetWindowPos(m_pWindow->GetHWND(), NULL, rcPos.left, rcPos.top, rcPos.right - rcPos.left, 
				rcPos.bottom - rcPos.top, SWP_NOZORDER | SWP_NOACTIVATE);        
		}
	}

	void CDateTimeUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
	{
		if(lstrcmpi(pstrName, _T("timeformat")) == 0) SetTimeFormat(pstrValue);
		else if(lstrcmpi(pstrName, _T("readonly")) == 0) SetReadOny(lstrcmpi(pstrValue, _T("true")) == 0);
		else return CLabelUI::SetAttribute(pstrName, pstrValue);
	}
}
