
template<typename T, typename TMod> CSingleColDb<T, TMod>* CSingleColDb<T, TMod>::NewL(
	MApp_context& Context, RDbDatabase& Db, const TDesC& TableName)
{
	auto_ptr< CSingleColDb<T, TMod> > ret(new (ELeave) CSingleColDb<T, TMod>(Context, Db));
	ret->ConstructL(TableName);
	return ret.release();
}

template<typename T, typename TMod> CSingleColDb<T, TMod>::~CSingleColDb()
{
}

template<typename T, typename TMod> bool CSingleColDb<T, TMod>::GetValueL(TInt Idx, TMod& Value)
{
	bool found=SeekL(Idx);
	if (!found) return false;
	if (iTable.IsColNull(2)) return false;
	ReadValueL(Value);
	return true;
}

template<typename T, typename TMod> void CSingleColDb<T, TMod>::SetValueL(TInt Idx, const T& Value)
{
	SeekL(Idx, true, true);
	WriteValue(Value);
	PutL();
}

template<typename T, typename TMod> CSingleColDb<T, TMod>::CSingleColDb(MApp_context& Context, 
	RDbDatabase& Db) : CSingleColDbBase(Context, Db)
{
}

template<typename T, typename TMod> void CSingleColDb<T, TMod>::ConstructL(const TDesC& TableName)
{
	CSingleColDbBase::ConstructL(TableName, ColType());
}

template<typename T, typename TMod> int  CSingleColDb<T, TMod>::ColType() const
{
	return EDbColInt32;
}

template<>
int  CSingleColDb<TDesC, TDes>::ColType() const
{
	return EDbColText;
}

template<>
int  CSingleColDb<TDesC8, TDes8>::ColType() const
{
	return EDbColText8;
}

template<>
int  CSingleColDb<TTime, TTime>::ColType() const
{
	return EDbColDateTime;
}

template<typename T, typename TMod> void CSingleColDb<T, TMod>::ReadValueL(TMod& Value)
{
	TInt32 v;
	v=iTable.ColInt(2);
	Value=T(v);
}

template<>
void CSingleColDb<TTime, TTime>::ReadValueL(TTime& Value)
{
	Value=iTable.ColTime(2);
}

template<>
void CSingleColDb<TDesC, TDes>::ReadValueL(TDes& Value)
{
	Value=iTable.ColDes16(2);
}

template<>
void CSingleColDb<TDesC8, TDes8>::ReadValueL(TDes8& Value)
{
	Value=iTable.ColDes8(2);
}

template<typename T, typename TMod> void CSingleColDb<T, TMod>::WriteValue(const T& Value)
{
	iTable.SetColL(2, TInt32(Value));
}

template<>
void CSingleColDb<TTime, TTime>::WriteValue(const TTime& Value)
{
	iTable.SetColL(2, Value);
}


template<>
void CSingleColDb<TDesC, TDes>::WriteValue(const TDesC& Value)
{
	iTable.SetColL(2, Value);
}

template<>
void CSingleColDb<TDesC8, TDes8>::WriteValue(const TDesC8& Value)
{
	iTable.SetColL(2, Value);
}

template<typename T, typename TMod> void CSingleColDb<T, TMod>::GetL(TUint& Idx, TMod& Value)
{
	iTable.GetL();
	Idx=iTable.ColUint32(1);
	ReadValueL(Value);
}
