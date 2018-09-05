#ifndef _STATES_H_
#define _STATES_H_


//////////////////////////////////////////////////////////////////////////////
//

enum ElementStates
{
	STATE_UNDEF = 0,
	STATE_ERROR = 1,
	STATE_WARNING = 2,
	STATE_OK = 3,
};

//////////////////////////////////////////////////////////////////////////////
//  The following values comes from the TDMF driver.
//  Please don't change them... instead you have a good reason of doing so.

//////////////////////////////////////////////////////////////////////////////
// Connection Status

#define  FTD_PMD_ONLY    (0)
#define  FTD_CONNECTED   (1)
#define  FTD_ACCUMULATE (-1)
#define  FTD_UNDEF       (2)

//////////////////////////////////////////////////////////////////////////////
// MODES

#define FTD_M_UNDEF         0x00
#define FTD_M_JNLUPDATE     0x01
#define FTD_M_BITUPDATE     0x02
#define FTD_MODE_PASSTHRU   0x10
#define FTD_MODE_NORMAL     FTD_M_JNLUPDATE
#define FTD_MODE_TRACKING   FTD_M_BITUPDATE
#define FTD_MODE_REFRESH    (FTD_M_JNLUPDATE | FTD_M_BITUPDATE)
#define FTD_MODE_BACKFRESH  0x20
#define FTD_MODE_STARTED    0x100
#define FTD_MODE_CHECKPOINT 0x200


//////////////////////////////////////////////////////////////////////////////
//

class CElementState
{
protected:
	enum ElementStates m_eState;

public:
	CElementState() : m_eState(STATE_UNDEF) {}
	CElementState(enum ElementStates eState) : m_eState(eState) {}
	CElementState(long nMode, long nConnectionState)
	{
		if (   (nMode != FTD_M_UNDEF) 
			&& (   (nMode            == FTD_MODE_TRACKING)
			    || (nMode            == FTD_MODE_PASSTHRU)
			    || (nConnectionState == FTD_ACCUMULATE)
			    || (nConnectionState == FTD_PMD_ONLY)
			   )
		   )
		{
			m_eState = STATE_ERROR;
		}
		else if (   (nConnectionState == FTD_CONNECTED)
			     && (   (nMode == FTD_MODE_REFRESH)
				     || (nMode == FTD_MODE_BACKFRESH)
					 || (nMode == FTD_MODE_CHECKPOINT)
					)
				 
				)
		{
			m_eState = STATE_WARNING;
		}
		else if ((nMode == FTD_MODE_NORMAL) && (nConnectionState == FTD_CONNECTED))
		{
			m_eState = STATE_OK;
		}
		else
		{
			m_eState = STATE_UNDEF;
		}
	}

	enum ElementStates State() {return m_eState;}

	CElementState& operator+=(const CElementState& ElementState)
	{
		this->operator+=(ElementState.m_eState);

		return *this;
	}

	CElementState& operator+=(enum ElementStates eState)
	{
		if (eState == STATE_ERROR)
		{
			m_eState = STATE_ERROR;
		}
		else if ((eState == STATE_WARNING) && (m_eState != STATE_ERROR))
		{
			m_eState = STATE_WARNING;
		}
		else if ((eState == STATE_OK) && (m_eState == STATE_UNDEF))
		{
			m_eState = STATE_OK;
		}

		return *this;
	}

};


#endif