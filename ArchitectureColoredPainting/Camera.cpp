#include "Camera.h"



// constructor with vectors
Camera::Camera(QVector3D position, QVector3D up, float yaw, float pitch) : Front(QVector3D(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
	Position = position;
	WorldUp = up;
	Yaw = yaw;
	Pitch = pitch;
	updateCameraVectors();
}
// constructor with scalar values
Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(QVector3D(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
	Position = QVector3D(posX, posY, posZ);
	WorldUp = QVector3D(upX, upY, upZ);
	Yaw = yaw;
	Pitch = pitch;
	updateCameraVectors();
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
QMatrix4x4 Camera::GetViewMatrix()
{
	QMatrix4x4 viewMatrix;
	viewMatrix.lookAt(Position, Position + Front, Up);
	return viewMatrix;
}

// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
	float velocity = MovementSpeed * deltaTime;
	if (direction == FORWARD)
		Position += PositionFront * velocity;
	if (direction == BACKWARD)
		Position -= PositionFront * velocity;
	if (direction == LEFT)
		Position -= PositionRight * velocity;
	if (direction == RIGHT)
		Position += PositionRight * velocity;
	if (direction == UP)
		Position.setY(Position.y() + velocity);
	if (direction == DOWN)
		Position.setY(Position.y() - velocity);
}

// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
	xoffset *= MouseSensitivity;
	yoffset *= MouseSensitivity;

	Yaw += xoffset;
	Pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (Pitch > 89.99f)
			Pitch = 89.99f;
		if (Pitch < -89.99f)
			Pitch = -89.99f;
	}

	// update Front, Right and Up Vectors using the updated Euler angles
	updateCameraVectors();
}

// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset)
{
	Zoom -= (float)yoffset*Zoom/60.;
	if (Zoom < 1.0f)
		Zoom = 1.0f;
	if (Zoom > 75.0f)
		Zoom = 75.0f;
}

// calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors()
{
	// calculate the new Front vector
	QVector3D front;
	front.setX(cos(qDegreesToRadians(Yaw)) * cos(qDegreesToRadians(Pitch)));
	front.setY(sin(qDegreesToRadians(Pitch)));
	front.setZ(sin(qDegreesToRadians(Yaw)) * cos(qDegreesToRadians(Pitch)));
	Front = front.normalized();
	// also re-calculate the Right and Up vector
	Right = QVector3D::crossProduct(Front, WorldUp).normalized();  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	Up = QVector3D::crossProduct(Right, Front).normalized();
	PositionFront = QVector3D(front.x(), 0.0f, front.z()).normalized();
	PositionRight = QVector3D::crossProduct(PositionFront, WorldUp).normalized();
}