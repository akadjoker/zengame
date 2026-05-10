#include "NodeEmitter2D.hpp"
#include "RenderUtils.hpp"
#include "Assets.hpp"

NodeEmitter2D::NodeEmitter2D(const std::string &graph,int numParticles,bool oneshot):Node2D("Emitter")
{
	this->graph =  Assets::Instance().getGraph(graph);
    this->NumParticles = numParticles;
    this->OneShot = oneshot;
    Particles = new Particle*[numParticles];
    for (int i = 0; i < numParticles; i++)
    {
        Particles[i] = new Particle();
    }
    
	X = 0;
	Y = 0;
	VX = 0;
	VY = 0;
	Angle = 10.0f;
	VMin = 1.0f;
	VMax = 1.0f;
	Size = 1;
    Life = 3.0f;
	InterpolateColor = true;
	Freq = 10;
	NumStart = 0;
	CurrParticle = 0;

	AMin = 0;
	AMax = 0;
	FaceDir = false;
    
	MaxParticles = -1;
	if (OneShot)
		MaxParticles = numParticles;
	EmittedParticles = 0;
	SomeAlive = false;
    
    

    colorStart.g=255;
    colorStart.r=255;
    colorStart.b=255;
    colorStart.time=0;

    colorEnd.g=255;
    colorEnd.r=255;
    colorEnd.b=255;
    colorEnd.time=0;

    sizeStart.time=0;
    sizeStart.value=0;

    sizeEnd.time=0;
    sizeEnd.value=0;

    alphaStart.value=0;
    alphaStart.time=0;

    alphaEnd.value=0;
    alphaEnd.time=0;


	StartX1 = 0;
	StartY1 = 0;
	StartX2 = 0;
	StartY2 = 0;

	getWorldTransform();
    
}

NodeEmitter2D::NodeEmitter2D(const std::string &name,const std::string &graph,int numParticles,bool oneshot):Node2D(name)
{
	this->graph =  Assets::Instance().getGraph(graph);
    this->NumParticles = numParticles;
    this->OneShot = oneshot;
    Particles = new Particle*[numParticles];
    for (int i = 0; i < numParticles; i++)
    {
        Particles[i] = new Particle();
    }
    X = 0;
	Y = 0;
	VX = 0;
	VY = 0;
	Angle = 10.0f;
	VMin = 1.0f;
	VMax = 1.0f;
	Size = 1;
    Life = 3.0f;
	InterpolateColor = true;
	Freq = 10;
	NumStart = 0;
	CurrParticle = 0;

	AMin = 0;
	AMax = 0;
	FaceDir = false;
    
	MaxParticles = -1;
	if (OneShot)
		MaxParticles = numParticles;
	EmittedParticles = 0;
	SomeAlive = false;
    
    
    colorStart.g=255;
    colorStart.r=255;
    colorStart.b=255;
    colorStart.time=0;

    colorEnd.g=255;
    colorEnd.r=255;
    colorEnd.b=255;
    colorEnd.time=0;

    sizeStart.time=0;
    sizeStart.value=0;

    sizeEnd.time=0;
    sizeEnd.value=0;

    alphaStart.value=0;
    alphaStart.time=0;

    alphaEnd.value=0;
    alphaEnd.time=0;


	StartX1 = 0;
	StartY1 = 0;
	StartX2 = 0;
	StartY2 = 0;
}

NodeEmitter2D::~NodeEmitter2D()
{
    for (int i = 0; i < NumParticles; i++)
    {
        delete Particles[i];
    }
    delete[] Particles;
}


void NodeEmitter2D::UpdateNumParticles()
{
	

	int iNewNum = ceil( Freq * Life ) + 2;  //final addition is a buffer to leave some room for error.
	if ( iNewNum > NumParticles )
	{
		Particle **pNewParticles = new Particle*[ iNewNum ];
		if ( NumParticles > 0 && Particles )
		{
			// copy old particles to new list
			for (  int i = 0; i < CurrParticle; i++ )
			{
				pNewParticles[ i ] = Particles[ i ];
			}

			// important to create new particles in the gap between oldest and newest particle
			// m_iCurrParticle points to oldest particle and will be used in next emission.
			 int remaining = NumParticles - CurrParticle;
			for (  int i = CurrParticle; i < iNewNum-remaining; i++ )
			{
				pNewParticles[ i ] = new Particle();
			}

			// add oldest particles after new ones
			for (  int i = iNewNum-remaining; i < iNewNum; i++ )
			{
				pNewParticles[ i ] = Particles[ i - (iNewNum - NumParticles) ];
			}
		}
		else
		{
			for (  int i = 0; i < iNewNum; i++ )
			{
				pNewParticles[ i ] = new Particle();
			}
		}

		if ( Particles ) delete [] Particles;
		Particles = pNewParticles;
		NumParticles = iNewNum;
	}
}


void NodeEmitter2D::SetColorEnd(unsigned char r, unsigned char g, unsigned char b, float time)
{
    colorEnd.r = r;
    colorEnd.g = g;
    colorEnd.b = b;
    colorEnd.time = time;
}


void NodeEmitter2D::SetColorStart(unsigned char r, unsigned char g, unsigned char b, float time )
{
    colorStart.r = r;
    colorStart.g = g;
    colorStart.b = b;
    colorStart.time = time;
}


void NodeEmitter2D::SetSizeEnd(float size, float time )
{
    sizeEnd.value = size;
    sizeEnd.time = time;
}

void NodeEmitter2D::SetSizeStart(float size, float time )
{
    sizeStart.value = size;
    sizeStart.time = time;
}

void NodeEmitter2D::SetAlphaEnd(float alpha, float time )
{

    alphaEnd.value = alpha;
    alphaEnd.time = time;
}

void  NodeEmitter2D::SetAlphaStart(float alpha, float time )
{

    alphaStart.value = alpha;
    alphaStart.time = time;
}


void NodeEmitter2D::SetFrequency( float freq )
{
		if ( freq < 0 ) freq = 0;
	if ( freq > 500.0f ) freq = 500.0f;

	Freq = freq;
    UpdateNumParticles();
}
void NodeEmitter2D::SetStartZone( float x1, float y1, float x2, float y2 )
{

	if ( x1 > x2 )
	{
		float temp = x1;
		x1 = x2;
		x2 = temp;
	}

	if ( y1 > y2 )
	{
		float temp = y1;
		y1 = y2;
		y2 = temp;
	}

	StartX1 = x1;
	StartY1 = y1;
	StartX2 = x2;
	StartY2 = y2;
}

void NodeEmitter2D::SetDirection( float vx, float vy )
{

	VX = vx;
	VY = vy;
}



void NodeEmitter2D::SetSelocityRange( float v1, float v2 )
{

	if ( v1 < 0.001f ) v1 = 0.001f;
	if ( v2 < 0.001f ) v2 = 0.001f;

	if ( v2 < v1 )
	{
		float temp = v1;
		v1 = v2;
		v2 = temp;
	}

	VMin = v1;
	VMax = v2;
}

void NodeEmitter2D::SetAngle(float angle )
{   


        if ( angle < 0 ) 
        {
            angle = 0;
        }
	    if ( angle > 360 ) 
        {
            angle = 360;
        }
	    Angle = angle * PI / 180.0f;
    
}
void NodeEmitter2D::SetAngleRad(float angle )
{   

   
        if ( angle < 0 ) 
        {
            angle = 0;
        }
	    if ( angle > 2*PI ) 
        {
            angle = 2*PI;
        }

	    Angle = angle;
 

	
}


void NodeEmitter2D::ResetCounter()
{
	EmittedParticles = 0;
}

void NodeEmitter2D::SetByMatrix(bool flag)
{
	byMatrix = flag;
}

bool NodeEmitter2D::IsFinished()
{
	
	bool full_emitted= (bool)(!SomeAlive) && (EmittedParticles >= MaxParticles);
    return OneShot && full_emitted;
}


bool NodeEmitter2D::IsReachEnd()
{
	return (bool)(!SomeAlive) && (EmittedParticles >= MaxParticles);
	
}

void NodeEmitter2D::SetSize(float size )
{
    if ( size < 0.0f ) size = 0.0f;
	Size = size;
}
void NodeEmitter2D::SetMaxParticles(int max )
{
	MaxParticles = max;
}

void NodeEmitter2D::SetRotationRate( float a1, float a2)
{
    a1 = a1 * PI / 180.0f;
	a2 = a2 * PI / 180.0f;
    SetRotationRateRad(a1,a2);
}
void NodeEmitter2D::SetRotationRateRad( float a1, float a2)
{
   if ( a2 < a1 )
	{
		float temp = a1;
		a1 = a2;
		a2 = temp;
	}

	AMin = a1;
	AMax = a2;

}
void NodeEmitter2D::SetFaceDirection(bool flag )
{
	FaceDir =flag;
}

void NodeEmitter2D::Offset(float x, float y )
{
    for ( int i = 0; i < NumParticles; i++ )
	{
		if ( !Particles[ i ]->Alive ) 
            continue;

		Particles[ i ]->X += x;
		Particles[ i ]->Y += y;
	}
}

void NodeEmitter2D::SetEmitterPosition(float x, float y )
{
	X = x;
	Y = y;
}

void NodeEmitter2D::SetLife( float time )
{
    if ( time < 0.001f ) time = 0.001f;
	if ( time > 120.0f ) time = 120.0f;

	Life = time;
	
    UpdateNumParticles();
}
void NodeEmitter2D::SetOneShot()
{
    OneShot = true;
}




int  Random( )
{
	return GetRandomValue(0, 65535);
}



void NodeEmitter2D::OnDraw(View *view)
{
	Node2D::OnDraw(view);

    float fNewSize= Size ;
 //   fNewSize *= 0.1f;
	unsigned int count = 0;
	if ( CurrParticle > 0 )
	{
		for ( int i = (int)CurrParticle-1; i >= 0; i-- )
		{
			if ( !Particles[ i ]->Alive ) continue;

			
			float size = Particles[ i ]->Scale * fNewSize;
			float x =  Particles[ i ]->X;
			float y =  Particles[ i ]->Y;
			float angle =Particles[ i ]->Angle;
			
            

			Color c ;
            c.r = Particles[ i ]->r;
            c.g = Particles[ i ]->g;
            c.b = Particles[ i ]->b;
            c.a = Particles[ i ]->a;

			if (!byMatrix)
			{
				
				//	Log(LOG_INFO,"[PARTICLE] %f %f %f %f",x,y,size,angle);
					if (graph!=NULL)
					{
						RenderTextureRotateSizeRad(graph->texture, x,y,angle,size,false,false,c);
					}
					else
						DrawCircle(x,y,size*2,c);
			} else 
			{
				Particle *p = Particles[ i ];

				Matrix2D mat;
				
				Vector2 position;
				Vector2 origin;
				Vector2 scale;

				position.x = p->X;
				position.y = p->Y;

				origin.x = graph->width/2;
				origin.y = graph->height/2;

				scale.x = Particles[ i ]->Scale; 
				scale.y = Particles[ i ]->Scale;

				mat.Transform(position, origin, scale, p->Angle );
				Matrix2D local =  Matrix2DMult(mat, transform);
				RenderTransform(graph->texture, local,c, 0);

           
				
			}
            
          count++;
		}
	}

	// wrap around the array back to oldest particle
	for ( int i = (int)NumParticles-1; i >= (int)CurrParticle ; i-- )
	{
		if ( !Particles[ i ]->Alive ) continue;


			float x = Particles[ i ]->X;
            float y = Particles[ i ]->Y;
			float angle =Particles[ i ]->Angle;
            float size = Particles[ i ]->Scale * fNewSize;
            
            Color c ;
            c.r = Particles[ i ]->r;
            c.g = Particles[ i ]->g;
            c.b = Particles[ i ]->b;
            c.a = Particles[ i ]->a;


			if (!byMatrix)
			{
				
				
					if (graph!=nullptr)
					{
						RenderTextureRotateSizeRad(graph->texture, x,y,angle,size,false,false,c);
					}else 
						DrawCircle(x,y,size*2,c);
				//	Log(LOG_INFO,"[PARTICLE] %f %f %f %f",x,y,size,angle)
			} else 
			{
				Particle *p = Particles[ i ];

				Matrix2D mat;
				
				Vector2 position;
				Vector2 origin;
				Vector2 scale;

				position.x = p->X;
				position.y = p->Y;

				origin.x = graph->width/2;
				origin.y = graph->height/2;

				scale.x = Particles[ i ]->Scale; 
				scale.y = Particles[ i ]->Scale;

	

				mat.Transform(position, origin, scale, p->Angle );
				Matrix2D local =  Matrix2DMult(mat, transform);
				RenderTransform(graph->texture, local,c, 0);
			
           
			
			}

		count++;
	}


//	DrawText(TextFormat("%f %f - %f %f",bound.x1 ,bound.y1,bound.x2,bound.y2),position.x,position.y,20,RED);
}

void NodeEmitter2D::OnUpdate(double time)
{
	Node2D::OnUpdate(time);


	//if ( OneShot && IsFinished() ) return;

    Vector2 p;
	p =  transform.TransformCoords(X,Y);
	
	if (OneShot && IsFinished() ) 
	{
		done = true;
	  	return;
	} else
	{
		if (IsReachEnd())
			EmittedParticles=0;
	}

	
	

    if ( MaxParticles < 0 || EmittedParticles < MaxParticles )
	{
		NumStart += (Freq * time);
	}

//TraceLog(LOG_INFO, "[PARTICLE] NUM %d start %f  %d  %d",NumParticles,NumStart,EmittedParticles,MaxParticles );

	bound.x1 = 99999999;
	bound.y1 = 99999999;
	bound.x2 = -99999999;
	bound.y2 = -99999999;


	while ( NumStart >= 1 )
	{
		// find the start point for this particle, could be anywhere within a box
		float x = StartX1;
		float y = StartY1;
		if ( StartX2 > StartX1 ) 
            x = (Random() / 65535.0f) * (StartX2 - StartX1) + StartX1;
		if ( StartY2 > StartY1 ) 
            y = (Random() / 65535.0f) * (StartY2 - StartY1) + StartY1;

		

		//Log(LOG_INFO,"[PARTICLE] %f %f %d",x,y,CurrParticle);
		Particles[ CurrParticle ]->X = p.x + x;
		Particles[ CurrParticle ]->Y = p.y + y;



		
		  
		bound.Encapsulate(Particles[ 0 ]->X, Particles[ 0 ]->Y);
	
		//bound.Expand( Particles[ CurrParticle ]->X, Particles[ CurrParticle ]->Y );
		


		
		// find the start angle for this particle, could be anywhere +/- Angle/2 radians from the emitter direction
		float vx = VX;
		float vy = VY;

     

		if ( Angle > 0 )
		{

     
	   	    
	 		float angle = Angle*(Random()/65535.0f - 0.5f);
            float cosA = cos( angle );
			float sinA = sin( angle );
    	    vx = (VX*cosA - VY*sinA);
			vy = (VY*cosA + VX*sinA);

            
		}
	// adjust the length of the direction vector (speed)
		if ( VMin != 1 || VMax != 1 )
		{
			float velocity = VMin + (Random() / 65535.0f) * (VMax - VMin);
			vx *= velocity;
			vy *= velocity;

		}

		// adjust the rotation speed
		Particles[ CurrParticle ]->Angle = 0;
		Particles[ CurrParticle ]->AngleDelta = 0;
		if ( AMin != 0 || AMax != 0 )
		{
			Particles[ CurrParticle ]->AngleDelta = AMin + (Random() / 65535.0f) * (AMax - AMin);
		}

		if ( FaceDir )
		{
			Particles[ CurrParticle ]->Angle = atan2( vy, vx );
		}

		 Particles[ CurrParticle ]->VX = vx;
		 Particles[ CurrParticle ]->VY = vy;
		 Particles[ CurrParticle ]->Time = 0;

	


	

		// make it active
		Particles[ CurrParticle ]->Alive = true;

		CurrParticle++;
		if ( CurrParticle >= NumParticles ) 
        {
            CurrParticle = 0;
        }
		NumStart--;

		EmittedParticles++;
		index++;
	}

	SomeAlive = false;



	// update live particles
	for ( int i = 0; i < NumParticles; i++ )
	{
		if ( !Particles[ i ]->Alive ) 
            continue;

		SomeAlive = true;

		Particles[ i ]->Time += time;
		if ( Particles[ i ]->Time > Life ) 
        {
            Particles[ i ]->Alive = false;
        }

	

		// modify angle
		if ( FaceDir )
		{
			Particles[ i ]->Angle = atan2( Particles[ i ]->VY, Particles[ i ]->VX ) + PI/2;
		}
		else
		{
			Particles[ i ]->Angle += Particles[ i ]->AngleDelta*time;
		}


		Particles[ i ]->X += Particles[ i ]->VX*time;
		Particles[ i ]->Y += Particles[ i ]->VY*time;


		



        {//colors
                float curr  = Particles[i]->Time - colorStart.time;
                float total = colorEnd.time - colorStart.time;

                if (total>0)
                {
                        float s = curr / total;
                        unsigned char r = (unsigned char)((1.0f - s) * colorStart.r + s * colorEnd.r);
                        unsigned char g = (unsigned char)((1.0f - s) * colorStart.g + s * colorEnd.g);
                        unsigned char b = (unsigned char)((1.0f - s) * colorStart.b + s * colorEnd.b);
                        Particles[i]->r=r;
                        Particles[i]->g=g;
                        Particles[i]->b=b;
                }
        }
        {//alpha
                float curr  = Particles[i]->Time - alphaStart.time;
                float total = alphaEnd.time - alphaStart.time;

                if (total>0)
                {
                        float s = curr / total;
                        unsigned char a = (unsigned char)((1.0f - s) * alphaStart.value + s * alphaEnd.value);
                        Particles[i]->a=a;
                }
        }
        {//size
                float curr  = Particles[i]->Time - sizeStart.time;
                float total = sizeEnd.time - sizeStart.time;

                if (total>0)
                {
                        float s = curr / total;
                        float size = ((1.0f - s) * sizeStart.value + s * sizeEnd.value);
                         Particles[i]->Scale=size;
                }

        }


		float w =0;
		float h =0;
		if (graph!=nullptr)
		{
			w = graph->width   * 0.5f;
			h = graph->height  * 0.5f;
		}
		
		  
		bound.Encapsulate(Particles[ i ]->X, Particles[ i ]->Y);
		bound.Encapsulate(Particles[ i ]->X +w, Particles[ i ]->Y);
		bound.Encapsulate(Particles[ i ]->X +w , Particles[ i ]->Y +h);
		bound.Encapsulate(Particles[ i ]->X , Particles[ i ]->Y +h);
		bound.Encapsulate(Particles[ i ]->X -w , Particles[ i ]->Y -h);
		bound.Encapsulate(Particles[ i ]->X  , Particles[ i ]->Y -h);
		
    }

}
