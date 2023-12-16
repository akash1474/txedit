// #include "imgui.h"
#pragma once


struct Coordinates {
	int mLine, mColumn;
	Coordinates() : mLine(0), mColumn(0) {}
	Coordinates(int aLine, int aColumn) : mLine(aLine), mColumn(aColumn)
	{
		// assert(aLine >= 0);
		// assert(aColumn >= 0);
	}
	static Coordinates Invalid()
	{
		static Coordinates invalid(-1, -1);
		return invalid;
	}

	bool operator==(const Coordinates& o) const { return mLine == o.mLine && mColumn == o.mColumn; }

	bool operator!=(const Coordinates& o) const { return mLine != o.mLine || mColumn != o.mColumn; }

	bool operator<(const Coordinates& o) const
	{
		if (mLine != o.mLine) return mLine < o.mLine;
		return mColumn < o.mColumn;
	}

	bool operator>(const Coordinates& o) const
	{
		if (mLine != o.mLine) return mLine > o.mLine;
		return mColumn > o.mColumn;
	}

	bool operator<=(const Coordinates& o) const
	{
		if (mLine != o.mLine) return mLine < o.mLine;
		return mColumn <= o.mColumn;
	}

	bool operator>=(const Coordinates& o) const
	{
		if (mLine != o.mLine) return mLine > o.mLine;
		return mColumn >= o.mColumn;
	}
};