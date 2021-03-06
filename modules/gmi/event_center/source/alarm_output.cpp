#include "alarm_output.h"

#if defined( __linux__ )
#include "gmi_brdwrapperdef.h"
#include "gmi_brdwrapper.h"
#endif

AlarmOutput::AlarmOutput( uint32_t EventProcessorId, uint32_t Index )
    : EventProcessor( EventProcessorId, Index )
    , m_OutputNumber( 0 )
    , m_Name()
    //, m_WorkMode( e_AlarmOutputWorkMode_DelayAutoTrigger )
    , m_DelayTime( 0 )
    , m_ScheduleTimes()
    , m_OperationLock()
{
	memset(m_TriggedTime, 0, sizeof(uint32_t)*MAX_NUM_EVENT_TYPE);
}

AlarmOutput::~AlarmOutput(void)
{
}

GMI_RESULT  AlarmOutput::SetName( const char_t *Name )
{
    if ( NULL == Name )
    {
        m_Name = NULL;
    }
    else
    {
        size_t NameLength = strlen( Name );
        m_Name = BaseMemoryManager::Instance().News<char_t>(NameLength+1);
        if ( NULL == m_Name.GetPtr() )
        {
            return GMI_OUT_OF_MEMORY;
        }
        if ( 0 < NameLength )
        {
            memcpy( m_Name.GetPtr(), Name, NameLength );
        }
        (m_Name.GetPtr())[NameLength] = 0;
    }

    return GMI_SUCCESS;
}

GMI_RESULT  AlarmOutput::AddScheduleTime( const ScheduleTimeInfo *Schedule )
{
    m_ScheduleTimes.push_back( *Schedule );
    return GMI_SUCCESS;
}

GMI_RESULT  AlarmOutput::ListScheduleTime( uint32_t *ItemNumber, ScheduleTimeInfo *Schedule )
{
    if ( *ItemNumber < m_ScheduleTimes.size() )
    {
        return GMI_NOT_ENOUGH_SPACE;
    }

    *ItemNumber = (uint32_t) m_ScheduleTimes.size();
    if ( 0 < *ItemNumber )
    {
        for ( uint32_t i = 0; i < *ItemNumber; ++i )
        {
            *Schedule = m_ScheduleTimes[i];
        }
    }
    return GMI_SUCCESS;
}

GMI_RESULT  AlarmOutput::Notify( uint32_t EventId, uint32_t Index, enum EventType Type, void_t *Parameter, size_t ParameterLength )
{
    std::vector<struct DetectorInfo>::iterator DetectorIdIt = m_DetectorIds.begin(), DetectorIdEnd = m_DetectorIds.end();
	GMI_RESULT Result = GMI_SUCCESS;
	int32_t BreakFlag = 0;
	time_t  CurrTime;
	CurrTime = time(NULL);

	
	m_OperationLock.Lock( TIMEOUT_INFINITE );
	for ( ; DetectorIdIt != DetectorIdEnd ; ++DetectorIdIt )
    {
    	
		if( ((*DetectorIdIt).s_DetectorId == EventId)
				&& ((0 < EventId) && (EventId <= MAX_NUM_EVENT_TYPE)) )
		{
			switch(EventId)
			{
				case EVENT_DETECTOR_ID_ALARM_INPUT:
					printf("Index=%d, s_IoNum=%d, GetOutputNumber=%d\n", Index, g_CurStartedAlarmIn[Index].s_AlarmInputInfo.s_LinkAlarmExtInfo.s_IoNum, GetOutputNumber());
					if((0 < (g_CurStartedAlarmIn[Index].s_AlarmInputInfo.s_LinkAlarmStrategy & (1<<(EVENT_PROCESSOR_ID_ALARM_OUTPUT-1))))
						&& ((g_CurStartedAlarmIn[Index].s_AlarmInputInfo.s_LinkAlarmExtInfo.s_IoNum & (1<<GetOutputNumber())) > 0))
					{
						if(e_AlarmOutputStatus_Opened == g_CurStartedAlarmOut[GetOutputNumber()].s_AlarmOutputInfo.s_NormalStatus)
						{
							Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_GPIO, GetOutputNumber(), (e_EventType_Start == Type) ? (uint8_t)e_AlarmOutputStatus_Closed: (uint8_t)e_AlarmOutputStatus_Opened );
						}
						else
						{
							Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_GPIO, GetOutputNumber(), (e_EventType_Start == Type) ? (uint8_t)e_AlarmOutputStatus_Opened: (uint8_t)e_AlarmOutputStatus_Closed);
						}
						if ( FAILED( Result ) )
			            {
							m_OperationLock.Unlock();
			                return Result;
			            }
			          

						if(e_EventType_Start == Type)
						{
							BreakFlag = 1;
						}
						else
						{
							BreakFlag = 2;
						}
					}
					break;
					
				case EVENT_DETECTOR_ID_HUMAN_DETECT:					
					printf("EventId=%d, s_IoNum=%d, GetOutputNumber=%d\n", EventId, g_CurStartedEvent[EventId-1].s_AlarmEventConfigInfo.s_LinkAlarmExtInfo.s_IoNum, GetOutputNumber());
					if(0 < (g_CurStartedEvent[EventId-1].s_AlarmEventConfigInfo.s_LinkAlarmStrategy & (1<<(EVENT_PROCESSOR_ID_ALARM_OUTPUT-1)))
						&& ((g_CurStartedEvent[EventId-1].s_AlarmEventConfigInfo.s_LinkAlarmExtInfo.s_IoNum & (1<<GetOutputNumber())) > 0))
			        {
						if(e_AlarmOutputStatus_Opened == g_CurStartedAlarmOut[GetOutputNumber()].s_AlarmOutputInfo.s_NormalStatus)
						{
							fprintf(stderr, "EVENT_DETECTOR_ID_HUMAN_DETECT trigger alarmout %d, normael status %d, need change status to %d\n",
								GetOutputNumber(), e_AlarmOutputStatus_Opened, e_AlarmOutputStatus_Closed);
							Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_GPIO, GetOutputNumber(), (e_EventType_Start == Type) ? (uint8_t)e_AlarmOutputStatus_Closed: (uint8_t)e_AlarmOutputStatus_Opened );
						}
						else
						{
							fprintf(stderr, "EVENT_DETECTOR_ID_HUMAN_DETECT trigger alarmout %d, normael status %d, need change status to %d\n",
								GetOutputNumber(), e_AlarmOutputStatus_Closed, e_AlarmOutputStatus_Opened);
							Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_GPIO, GetOutputNumber(), (e_EventType_Start == Type) ? (uint8_t)e_AlarmOutputStatus_Opened: (uint8_t)e_AlarmOutputStatus_Closed);
						}
						if ( FAILED( Result ) )
			            {	            	
							m_OperationLock.Unlock();
			                return Result;
			            }			            

						if(e_EventType_Start == Type)
						{
							BreakFlag = 1;
						}
						else
						{
							BreakFlag = 2;
						}
			        }
					break;
				default:
					fprintf(stderr, "AlarmOutput::Notify EventId %d error.\n", EventId);
					break;
			}
			
		}
		if(0 <  BreakFlag)
		{
			if(1 == BreakFlag)
			{
				SetTriggedTime(CurrTime, EventId);
			}
			else if(2 == BreakFlag)
			{
				SetTriggedTime(0, EventId);
			}
			break;
		}
    }
	m_OperationLock.Unlock();

    return GMI_SUCCESS;
}

GMI_RESULT  AlarmOutput::Start( const void_t *Parameter, size_t ParameterLength )
{
    const struct AlarmOutputInfo *Info = (const struct AlarmOutputInfo *) Parameter;
    SetOutputNumber( Info->s_OutputNumber );
    SetName( Info->s_Name );
    //SetWorkMode( (enum AlarmOutputWorkMode) Info->s_WorkMode );
    SetDelayTime( Info->s_DelayTime );
	#if 0
    for ( uint32_t i = 0; i < Info->s_ScheduleTimeNumber; ++i )
    {
        AddScheduleTime( &(Info->s_ScheduleTime[i]) );
    }
	#endif

	GMI_RESULT Result = m_TimerThread.Create( NULL, 0, TimerThread, this );
    if ( FAILED( Result ) )
    {
        AlarmOutput::Stop();
        return Result;
    }

    Result = m_TimerThread.Start();
    if ( FAILED( Result ) )
    {
        m_TimerThread.Destroy();
        AlarmOutput::Stop();
        return Result;
    }

	Result = m_OperationLock.Create( NULL );
    if ( FAILED( Result ) )
    {
        m_TimerThread.Destroy();
        AlarmOutput::Stop();
        return Result;
    }

    return GMI_SUCCESS;
}

GMI_RESULT  AlarmOutput::Stop()
{
	m_ThreadExitFlag = true;
    while ( m_ThreadWorking ) GMI_Sleep( 10 );
    m_TimerThread.Destroy();
	m_OperationLock.Destroy();

    return GMI_SUCCESS;
}

void_t* AlarmOutput::TimerThread( void_t *Argument )
{
    AlarmOutput *TimerOperate = reinterpret_cast<AlarmOutput*> ( Argument );
    void_t *Return = TimerOperate->TimerEntry();
    return Return;
}

void_t* AlarmOutput::TimerEntry()
{
    GMI_RESULT Result = GMI_FAIL;
    m_ThreadWorking   = true;

    //uint8_t GPIOStatus = 0;
	//int8_t s_GPIOStatus = 0;
	time_t       CurrTime;
	int32_t i = 0;

    while( !m_ThreadExitFlag )
    {
    	CurrTime = time(NULL);
		if(GetDelayTime() != g_CurStartedAlarmOut[GetOutputNumber()].s_AlarmOutputInfo.s_DelayTime)
		{
			SetDelayTime(g_CurStartedAlarmOut[GetOutputNumber()].s_AlarmOutputInfo.s_DelayTime);
		}
		m_OperationLock.Lock( TIMEOUT_INFINITE );
		for(i=1; i <= MAX_NUM_EVENT_TYPE; i++)
		{
			if((GetTriggedTime(i) > 0)
				&&((CurrTime > (GetTriggedTime(i)+GetDelayTime())) || (GetTriggedTime(i)> (uint32_t)CurrTime)))
			{
				switch(i)
				{
					case EVENT_DETECTOR_ID_HUMAN_DETECT:
					case EVENT_DETECTOR_ID_ALARM_INPUT:
						printf("GMI_ALARM_MODE_GPIO recover[%u]\n", (uint32_t)CurrTime);
			            //Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_LIGHT, 0, 0 );
						if(e_AlarmOutputStatus_Opened == g_CurStartedAlarmOut[GetOutputNumber()].s_AlarmOutputInfo.s_NormalStatus)
						{
							fprintf(stderr, "recover trigger alarmout %d, normael status %d, need recover status to %d\n",
								GetOutputNumber(), e_AlarmOutputStatus_Opened, e_AlarmOutputStatus_Opened);
							Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_GPIO, GetOutputNumber(), (uint8_t)e_AlarmOutputStatus_Opened );
						}
						else
						{
							fprintf(stderr, "recover trigger alarmout %d, normael status %d, need recover status to %d\n",
								GetOutputNumber(), e_AlarmOutputStatus_Closed, e_AlarmOutputStatus_Closed);
							Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_GPIO, GetOutputNumber(), (uint8_t)e_AlarmOutputStatus_Closed );
						}
						if ( FAILED( Result ) )
			            {	            	
						    fprintf(stderr, "GMI_BrdSetAlarmOutput GMI_ALARM_MODE_GPIO fail\n");
			            }
						break;
					default:
						break;
				}
				SetTriggedTime(0, i);
			}
		}
		m_OperationLock.Unlock();
        GMI_Sleep( 1000 );
    }
	for(i=1; i <= MAX_NUM_EVENT_TYPE; i++)
	{
		if(GetTriggedTime(i) > 0)
		{
			switch(i)
			{
				case EVENT_DETECTOR_ID_HUMAN_DETECT:
					#if 0
					printf("GMI_ALARM_MODE_LIGHT close[%u]\n", (uint32_t)CurrTime);
		            Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_LIGHT, 0, 0 );
					if ( FAILED( Result ) )
		            {	            	
					    fprintf(stderr, "GMI_BrdSetAlarmOutput GMI_ALARM_MODE_LIGHT fail\n");
		            }
					#endif
					break;
				case EVENT_DETECTOR_ID_ALARM_INPUT:
		            //Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_LIGHT, 0, 0 );
					if(e_AlarmOutputStatus_Opened == g_CurStartedAlarmOut[GetOutputNumber()].s_AlarmOutputInfo.s_NormalStatus)
					{
						Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_GPIO, GetOutputNumber(), (uint8_t)e_AlarmOutputStatus_Opened );
					}
					else
					{
						Result = GMI_BrdSetAlarmOutput( GMI_ALARM_MODE_GPIO, GetOutputNumber(), (uint8_t)e_AlarmOutputStatus_Closed );
					}
					if ( FAILED( Result ) )
		            {	            	
					    fprintf(stderr, "GMI_BrdSetAlarmOutput GMI_ALARM_MODE_GPIO fail\n");
		            }
					break;
				default:
					break;
			}
			SetTriggedTime(0, i);
		}
	}
    m_ThreadWorking   = false;
    return (void_t *) size_t(Result);
}

