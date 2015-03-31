/*

	struct point_array
	{
		point* Elements;
		size_t Size;
		size_t Capacity;
	}

	=== Declaration
	point[?] Points;
	 ~~>
	point_array Points;

	=== Assignment to end
	Points[>] = Point{2, 2};
	 ~~>
	Point P = {2, 2};
	if (Size + 1 >= Capacity) { Expand(Points); }
	Elements[Size] = P;

	=== Assignment to beginning
	Points[<] = Point{2, 2};
	 ~~>
	Point P = {2, 2};
	if (Size + 1 >= Capacity) { Expand(Points); }
	Copy(Points[0], Size, Points[1], Size);
	Points[0] = P;

	=== Sub-array (by copy) (Python-ish syntax)
	point[?] PtsMid = Points[2:6]; // Gives a copy with the same values as elements [2, 3, 4, 5, 6]
	point[?] PtsEnd = Points[2:]; // Gives a copy with the same values as elements [2, 3, 4, ..., Size]
	point[?] PtsSrt = Points[:6]; // Gives a copy with the asme values as elements [0, 1, 2, 3, ... 6]
	point[?] NewPts = Points[:]; // Returns an exact copy.
	

	
	==========
	Type need-to-knows

	 + What type am I assigning to this? (Know the type of any given symbol)
	 - What are the intermediate types and how do they convert?
	 - 



*/