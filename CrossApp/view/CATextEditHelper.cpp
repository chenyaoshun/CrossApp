#include "CATextEditHelper.h"
#include "basics/CAApplication.h"
#include "basics/CAScheduler.h"
#include "control/CAButton.h"
#include "CAAlertView.h"
#include "support/ccUTF8.h"
#include "platform/CCPlatformMacros.h"
#include "CAWindow.h"


#define CATextArrowViewWidth 30
#define CATextArrowViewHeight 62
#define CATextSelectArrWidth 40
#define CATextSelectArrHeight 64


NS_CC_BEGIN


CATouchView::CATouchView()
: m_pCurTouch(NULL)
, m_pCurEvent(NULL)
{
}

bool CATouchView::ccTouchBegan(CATouch *pTouch, CAEvent *pEvent)
{
	CAScheduler::schedule(schedule_selector(CATouchView::ccTouchTimer), this, 0, 0, 1.0f);

	m_pCurTouch = pTouch;
	m_pCurEvent = pEvent;
	return true;
}

void CATouchView::ccTouchMoved(CATouch *pTouch, CAEvent *pEvent)
{
	CAScheduler::unschedule(schedule_selector(CATouchView::ccTouchTimer), this);
}

void CATouchView::ccTouchEnded(CATouch *pTouch, CAEvent *pEvent)
{
	CAScheduler::unschedule(schedule_selector(CATouchView::ccTouchTimer), this);
}

void CATouchView::ccTouchCancelled(CATouch *pTouch, CAEvent *pEvent)
{
	CAScheduler::unschedule(schedule_selector(CATouchView::ccTouchTimer), this);
}


void CATouchView::ccTouchTimer(float interval)
{
	CAScheduler::unschedule(schedule_selector(CATouchView::ccTouchTimer), this);
	ccTouchPress(m_pCurTouch, m_pCurEvent);
}



////////////////////////////////////////////////////////////////////////////////////////////
CATextToolBarView::CATextToolBarView()
: m_pBackView(NULL)
{
}


CATextToolBarView::~CATextToolBarView()
{
}

void CATextToolBarView::addButton(const std::string& strBtnText, CAObject* target, SEL_CallFunc selector)
{
	m_CallbackTargets.push_back(CallbackTarget(target, selector, strBtnText));
}

void CATextToolBarView::show()
{
	CADipSize winSize = this->getBounds().size;

	float alertViewButtonHeight = 88;
	float alertViewWidth = winSize.width * 2 / 3;

	CADipRect rect = CADipRect(winSize.width / 2, winSize.height / 2, alertViewWidth, alertViewButtonHeight*m_CallbackTargets.size());

	m_pBackView = CAClippingView::create();
	m_pBackView->setCenter(rect);
	this->addSubview(m_pBackView);
	this->setTextTag("CATextToolBarView");
	m_pBackView->setAlphaThreshold(0.5f);

	CAScale9ImageView *backgroundImageView = CAScale9ImageView::createWithFrame(m_pBackView->getBounds());
	backgroundImageView->setImage(CAImage::create("source_material/alert_back.png"));
	m_pBackView->addSubview(backgroundImageView);
	m_pBackView->setStencil(backgroundImageView->copy());

	size_t btnCount = m_CallbackTargets.size();

	for (int i = 0; i < btnCount; i++) 
	{
		CAButton* btn = CAButton::create(CAButtonTypeSquareRect);
		btn->setTitleForState(CAControlStateAll, m_CallbackTargets[i].cszButtonText.c_str());
		btn->setTitleColorForState(CAControlStateAll, ccc4(3, 100, 255, 255));
		btn->setBackGroundViewForState(CAControlStateNormal, CAView::createWithColor(CAColor_clear));
		btn->setBackGroundViewForState(CAControlStateHighlighted, CAView::createWithColor(ccc4(226, 226, 226, 225)));
		btn->setTag(i);
		btn->addTarget(this, CAControl_selector(CATextToolBarView::alertViewCallback), CAControlEventTouchUpInSide);
		btn->setFrame(CADipRect(0, alertViewButtonHeight * i, alertViewWidth, alertViewButtonHeight));
		m_pBackView->addSubview(btn);

		if (i>0)
		{
			addGrayLine(alertViewButtonHeight * i);
		}
	}

	if (CAWindow *rootWindow = CAApplication::getApplication()->getRootWindow())
	{
		rootWindow->insertSubview(this, CAWindowZoderTop);
	}
	becomeFirstResponder();
}

bool CATextToolBarView::init()
{
	if (!CAView::init())
	{
		return false;
	}

	this->setColor(ccc4(135, 135, 135, 190));
	CCRect rect = CCRectZero;
	rect.size = CAApplication::getApplication()->getWinSize();
	this->setFrame(rect);

	return true;
}

CATextToolBarView *CATextToolBarView::create()
{
	CATextToolBarView *pAlert = new CATextToolBarView();
	if (pAlert && pAlert->init())
	{
		pAlert->autorelease();
		return pAlert;
	}
	CC_SAFE_DELETE(pAlert);
	return pAlert;
}

bool CATextToolBarView::isTextToolBarShow()
{
	bool isShow = false;
	if (CAWindow *rootWindow = CAApplication::getApplication()->getRootWindow())
	{
		isShow = rootWindow->getSubviewByTextTag("CATextToolBarView")!=NULL;
	}
	return isShow;
}


void CATextToolBarView::hideTextToolBar()
{
	CATextToolBarView* pToolBarView = NULL;
	if (CAWindow *rootWindow = CAApplication::getApplication()->getRootWindow())
	{
		pToolBarView = (CATextToolBarView*)rootWindow->getSubviewByTextTag("CATextToolBarView");
	}
	if (pToolBarView)
	{
		pToolBarView->resignFirstResponder();
		pToolBarView->removeFromSuperview();
	}
}


void CATextToolBarView::addGrayLine(int y) 
{
	CCSize size = CAApplication::getApplication()->getWinSize();
	CAView *line = createWithFrame(CADipRect(0, y, size.width, 1));
	line->setColor(ccc4(206, 206, 211, 255));
	m_pBackView->addSubview(line);
}

void CATextToolBarView::alertViewCallback(CAControl* btn, CCPoint point)
{
	resignFirstResponder();
	int btnIndex = btn->getTag();
	if (btnIndex>=0 && btnIndex<m_CallbackTargets.size())
	{
		this->retain();
		((CAObject*)m_CallbackTargets[btnIndex].target->*m_CallbackTargets[btnIndex].selector)();
		this->autorelease();
	}
	m_CallbackTargets.clear();
	removeFromSuperview();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
CATextSelectView::CATextSelectView()
: m_pCursorMarkL(NULL)
, m_pCursorMarkR(NULL)
, m_pTextViewMask(NULL)
, m_pControlView(NULL)
{

}

CATextSelectView::~CATextSelectView()
{

}

bool CATextSelectView::init()
{
	if (!CAView::init())
		return false;

	m_pTextViewMask = CAView::createWithColor(ccc4(60, 120, 240, 127));
	addSubview(m_pTextViewMask);
	m_pTextViewMask->setVisible(false);

	m_pCursorMarkL = CAImageView::createWithImage(CAImage::create("source_material/text_pos_l.png"));
	addSubview(m_pCursorMarkL);
	m_pCursorMarkL->setVisible(false);

	m_pCursorMarkR = CAImageView::createWithImage(CAImage::create("source_material/text_pos_r.png"));
	addSubview(m_pCursorMarkR);
	m_pCursorMarkR->setVisible(false);

	return true;
}

CATextSelectView *CATextSelectView::create()
{
	CATextSelectView *pTextSelView = new CATextSelectView();
	if (pTextSelView && pTextSelView->init())
	{
		pTextSelView->autorelease();
		return pTextSelView;
	}
	CC_SAFE_DELETE(pTextSelView);
	return pTextSelView;
}

void CATextSelectView::hideTextSelectView()
{
	CATextSelectView* pSelTextView = NULL;
	if (CAWindow *rootWindow = CAApplication::getApplication()->getRootWindow())
	{
		pSelTextView = (CATextSelectView*)rootWindow->getSubviewByTextTag("CATextSelectView");
	}
	if (pSelTextView)
	{
		pSelTextView->hideTextSelView();
	}
}

void CATextSelectView::showTextSelView(const CCRect& rect, CAView* pControlView, bool showLeft, bool showRight)
{
	if (getSuperview() != NULL)
		return;

	CCSize winSize = CAApplication::getApplication()->getWinSize();
	setFrame(CCRect(0, 0, winSize.width, winSize.height));
	setColor(CAColor_clear);
	setTextTag("CATextSelectView");

	CADipRect newRect = rect;
	if (showLeft)
	{
		m_pCursorMarkL->setFrame(CADipRect(newRect.origin.x - CATextSelectArrWidth, newRect.origin.y + newRect.size.height, CATextSelectArrWidth, CATextSelectArrHeight));
		m_pCursorMarkL->setVisible(true);
	}

	if (showRight)
	{
		m_pCursorMarkR->setFrame(CADipRect(newRect.origin.x + newRect.size.width, newRect.origin.y + newRect.size.height, CATextSelectArrWidth, CATextSelectArrHeight));
		m_pCursorMarkR->setVisible(true);
	}

	m_pTextViewMask->setFrame(newRect);
	m_pTextViewMask->setVisible(true);


	if (CAView *rootWindow = CAApplication::getApplication()->getRootWindow())
	{
		rootWindow->removeSubviewByTextTag("CATextSelectView");
		rootWindow->addSubview(this);
	}
	becomeFirstResponder();
	m_pControlView = pControlView;
}

void CATextSelectView::hideTextSelView()
{
	resignFirstResponder();

	if (m_pControlView)
	{
		m_pControlView->becomeFirstResponder();
		m_pControlView = NULL;
	}

	removeFromSuperview();

	CAApplication::getApplication()->updateDraw();
}

bool CATextSelectView::ccTouchBegan(CATouch *pTouch, CAEvent *pEvent)
{
	CCPoint cTouchPoint = this->convertTouchToNodeSpace(pTouch);

	CCRect newRectL = m_pCursorMarkL->getFrame();
	newRectL.InflateRect(5);
	CCRect newRectR = m_pCursorMarkR->getFrame();
	newRectR.InflateRect(5);

	m_iSelViewTouchPos = 0;
	if (newRectL.containsPoint(cTouchPoint))
	{
		m_iSelViewTouchPos = 1;
		return true;
	}

	if (newRectR.containsPoint(cTouchPoint))
	{
		m_iSelViewTouchPos = 2;
		return true;
	}

	CCPoint point = this->convertTouchToNodeSpace(pTouch);

	CCRect ccTextRect = m_pTextViewMask->getFrame();
	if (ccTextRect.containsPoint(point))
	{
		CATextToolBarView *pToolBar = CATextToolBarView::create();
		pToolBar->addButton(UTF8("\u526a\u5207"), this, callfunc_selector(CATextSelectView::ccCutToClipboard));
		pToolBar->addButton(UTF8("\u590d\u5236"), this, callfunc_selector(CATextSelectView::ccCopyToClipboard));
		pToolBar->addButton(UTF8("\u7c98\u8d34"), this, callfunc_selector(CATextSelectView::ccPasteFromClipboard));
		pToolBar->show();
	}
	else
	{
		if (resignFirstResponder())
		{
			hideTextSelView();
		}
		else
		{
			becomeFirstResponder();
		}
	}
	return true;
}

void CATextSelectView::ccTouchMoved(CATouch *pTouch, CAEvent *pEvent)
{
	if (m_iSelViewTouchPos != 1 && m_iSelViewTouchPos != 2)
		return;

	CAIMEDispatcher::sharedDispatcher()->dispatchMoveSelectChars(m_iSelViewTouchPos == 1, pTouch->getLocation());
}

void CATextSelectView::ccCopyToClipboard()
{
	hideTextSelView();
	CAIMEDispatcher::sharedDispatcher()->dispatchCopyToClipboard();
}

void CATextSelectView::ccCutToClipboard()
{
	hideTextSelView();
	CAIMEDispatcher::sharedDispatcher()->dispatchCutToClipboard();
}

void CATextSelectView::ccPasteFromClipboard()
{
	hideTextSelView();
	CAIMEDispatcher::sharedDispatcher()->dispatchPasteFromClipboard();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
CATextSelViewEx::CATextSelViewEx()
{

}

CATextSelViewEx::~CATextSelViewEx()
{

}

bool CATextSelViewEx::init()
{
	if (!CAView::init())
		return false;

	m_pCursorMarkL = CAImageView::createWithImage(CAImage::create("source_material/text_pos_l.png"));
	insertSubview(m_pCursorMarkL, CAWindowZoderTop);
	m_pCursorMarkL->setVisible(false);

	m_pCursorMarkR = CAImageView::createWithImage(CAImage::create("source_material/text_pos_r.png"));
	insertSubview(m_pCursorMarkR, CAWindowZoderTop);
	m_pCursorMarkR->setVisible(false);

	setHaveNextResponder(false);
	return true;
}

CATextSelViewEx *CATextSelViewEx::create()
{
	CATextSelViewEx *pTextSelView = new CATextSelViewEx();
	if (pTextSelView && pTextSelView->init())
	{
		pTextSelView->autorelease();
		return pTextSelView;
	}
	CC_SAFE_DELETE(pTextSelView);
	return pTextSelView;
}

void CATextSelViewEx::showTextViewMark(const std::vector<CCRect>& vt)
{
	hideTextViewMark();
	for (int i = 0; i < vt.size(); i++)
	{
		CAView* pTextMaskView = CAView::createWithColor(ccc4(60, 120, 240, 127));
		pTextMaskView->setFrame(vt[i]);
		addSubview(pTextMaskView);
		m_pTextViewMask.push_back(pTextMaskView);
	}
}

void CATextSelViewEx::hideTextViewMark()
{
	for (int i = 0; i < m_pTextViewMask.size(); i++)
	{
		removeSubview(m_pTextViewMask[i]);
	}
	m_pTextViewMask.clear();
}

void CATextSelViewEx::showTextSelView(const std::vector<CCRect>& vt, float iLineHeight)
{
	CAView* pControlView = getSuperview();
	if (pControlView == NULL)
		return;

	setFrame(pControlView->getFrame());
	setColor(CAColor_clear);
	showTextViewMark(vt);

	CCRect r = (vt.size() == 1) ? vt[0] : vt.back();

	CCPoint pt1 = vt[0].origin;
	CCPoint pt2 = CCPoint(r.origin.x + r.size.width, r.origin.y + r.size.height);

	m_pCursorMarkL->setFrame(CADipRect(pt1.x - CATextSelectArrWidth, pt1.y + iLineHeight, CATextSelectArrWidth, CATextSelectArrHeight));
	m_pCursorMarkL->setVisible(true);

	m_pCursorMarkR->setFrame(CCRect(pt2.x, pt2.y, CATextSelectArrWidth, CATextSelectArrHeight));
	m_pCursorMarkR->setVisible(true);

	becomeFirstResponder();
}

void CATextSelViewEx::hideTextSelView()
{
	hideTextViewMark();

	m_pCursorMarkL->setVisible(false);
	m_pCursorMarkR->setVisible(false);
}

bool CATextSelViewEx::isTextViewShow()
{
	return !m_pTextViewMask.empty();
}

bool CATextSelViewEx::ccTouchBegan(CATouch *pTouch, CAEvent *pEvent)
{
	CCPoint cTouchPoint = this->convertTouchToNodeSpace(pTouch);

	CCRect newRectL = m_pCursorMarkL->getFrame();
	newRectL.InflateRect(5);
	CCRect newRectR = m_pCursorMarkR->getFrame();
	newRectR.InflateRect(5);

	m_iSelViewTouchPos = 0;
	if (newRectL.containsPoint(cTouchPoint))
	{
		m_iSelViewTouchPos = 1;
		return true;
	}

	if (newRectR.containsPoint(cTouchPoint))
	{
		m_iSelViewTouchPos = 2;
		return true;
	}

	CCPoint point = this->convertTouchToNodeSpace(pTouch);

	bool isTouchTextView = false;
	for (int i = 0; i < m_pTextViewMask.size(); i++)
	{
		CCRect ccTextRect = m_pTextViewMask[i]->getFrame();
		if (ccTextRect.containsPoint(point))
		{
			isTouchTextView = true;
			break;
		}
	}

	if (isTouchTextView)
	{
		CATextToolBarView *pToolBar = CATextToolBarView::create();
		pToolBar->addButton(UTF8("\u526a\u5207"), this, callfunc_selector(CATextSelViewEx::ccCutToClipboard));
		pToolBar->addButton(UTF8("\u590d\u5236"), this, callfunc_selector(CATextSelViewEx::ccCopyToClipboard));
		pToolBar->addButton(UTF8("\u7c98\u8d34"), this, callfunc_selector(CATextSelViewEx::ccPasteFromClipboard));
		pToolBar->show();
	}
	else
	{
		resignFirstResponder();
	
	//	CAView* pControlView = getSuperview();
	//	if (pControlView)
	//	{
	//		pControlView->becomeFirstResponder();
	//	}
		hideTextSelView();
	}
	return true;
}

void CATextSelViewEx::ccTouchMoved(CATouch *pTouch, CAEvent *pEvent)
{
	if (m_iSelViewTouchPos != 1 && m_iSelViewTouchPos != 2)
		return;

	CAIMEDispatcher::sharedDispatcher()->dispatchMoveSelectChars(m_iSelViewTouchPos == 1, pTouch->getLocation());
}

void CATextSelViewEx::ccCopyToClipboard()
{
	hideTextSelView();
	CAIMEDispatcher::sharedDispatcher()->dispatchCopyToClipboard();
}

void CATextSelViewEx::ccCutToClipboard()
{
	hideTextSelView();
	CAIMEDispatcher::sharedDispatcher()->dispatchCutToClipboard();
}

void CATextSelViewEx::ccPasteFromClipboard()
{
	hideTextSelView();
	CAIMEDispatcher::sharedDispatcher()->dispatchPasteFromClipboard();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
CATextArrowView::CATextArrowView()
: m_pArrowView(NULL)
, m_isBtnPress(false)
{

}

CATextArrowView::~CATextArrowView()
{
	CAScheduler::unschedule(schedule_selector(CATextArrowView::ccTouchTimer), this);
}


bool CATextArrowView::init()
{
	if (!CAView::init())
		return false;

	m_pArrowView = CAImageView::createWithImage(CAImage::create("source_material/arrow.png"));
	addSubview(m_pArrowView);
	m_pArrowView->setVisible(false);
	m_pArrowView->setFrame(CCRectMake(0, 0, CATextArrowViewWidth, CATextArrowViewHeight));
	m_pArrowView->setAlpha(0.5f);

	return true;
}


CATextArrowView *CATextArrowView::create()
{
	CATextArrowView *pTextArrowView = new CATextArrowView();
	if (pTextArrowView && pTextArrowView->init())
	{
		pTextArrowView->autorelease();
		return pTextArrowView;
	}
	CC_SAFE_DELETE(pTextArrowView);
	return pTextArrowView;
}

bool CATextArrowView::ccTouchBegan(CATouch *pTouch, CAEvent *pEvent)
{
	CCPoint point = this->convertTouchToNodeSpace(pTouch);

	CCRect newRectR = m_pArrowView->getFrame();
	newRectR.InflateRect(5);

	if (!newRectR.containsPoint(point))
	{
		if (resignFirstResponder())
		{
			hideTextArrView();
		}
	}
	else
	{
		m_isBtnPress = true;
	}
	return true;
}

void CATextArrowView::ccTouchMoved(CATouch *pTouch, CAEvent *pEvent)
{
	if (m_isBtnPress)
	{
		CAIMEDispatcher::sharedDispatcher()->dispatchMoveArrowBtn(pTouch->getLocation());
	}

	CAView::ccTouchMoved(pTouch, pEvent);
}

void CATextArrowView::ccTouchEnded(CATouch *pTouch, CAEvent *pEvent)
{
	m_isBtnPress = false;
}

void CATextArrowView::showTextArrView(const CCPoint& pt)
{
	setFrame(this->getSuperview()->getFrame());
	setColor(CAColor_clear);
	m_pArrowView->setVisible(true);
	m_pArrowView->setCenterOrigin(CADipPoint(pt.x, pt.y));

	becomeFirstResponder();
	CAScheduler::schedule(schedule_selector(CATextArrowView::ccTouchTimer), this, 0, 0, 3);
}

void CATextArrowView::hideTextArrView()
{
	m_pArrowView->setVisible(false);
}

void CATextArrowView::ccTouchTimer(float interval)
{
	CAScheduler::unschedule(schedule_selector(CATextArrowView::ccTouchTimer), this);
	hideTextArrView();
}


NS_CC_END