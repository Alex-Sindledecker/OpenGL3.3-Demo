#include "CameraFP.h"

CameraFP::CameraFP(glm::vec3 pos, float walkSpeed)
{
	this->pos = pos;
	this->walkSpeed = walkSpeed;
	attitude = glm::vec3(0);
	front = glm::vec3(0);
	sprintButton = sf::Keyboard::LShift;
	sprintSpeed = 6;
	up = glm::vec3(0, 1, 0);
	view = glm::mat4(1.f);
	xBounds = glm::vec2(0);
	zBounds = glm::vec2(0);
}

CameraFP::~CameraFP()
{

}

void CameraFP::setBounds(glm::vec2 xBounds, glm::vec2 zBounds)
{
	this->xBounds = xBounds;
	this->zBounds = zBounds;
}

void CameraFP::setPosition(glm::vec3 pos)
{
	this->pos = pos;
}

void CameraFP::setPosition(float x, float y, float z)
{
	pos = glm::vec3(x, y, z);
}

void CameraFP::move(glm::vec3 displacement)
{
	pos += displacement;
}

void CameraFP::move(float dx, float dy, float dz)
{
	pos += glm::vec3(dx, dy, dz);
}

void CameraFP::inputProc(sf::Event& event, float dt, float mouseDx, float mouseDy)
{
	float speed = walkSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
		speed = sprintSpeed;

	if (pos.x < xBounds.x + 0.2)
		pos.x = xBounds.x + 0.2;
	else if (pos.x > xBounds.y - 0.2)
		pos.x = xBounds.y - 0.2;
	if (pos.z < zBounds.x + 0.2)
		pos.z = zBounds.x + 0.2;
	else if (pos.z > zBounds.y - 0.2)
		pos.z = zBounds.y - 0.2;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
	{
		pos += glm::normalize(glm::vec3(front.x, 0, front.z)) * dt * speed;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
	{
		pos -= glm::normalize(glm::vec3(front.x, 0, front.z)) * dt * speed;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
	{
		pos -= glm::normalize(glm::cross(front, up)) * dt * speed;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
	{
		pos += glm::normalize(glm::cross(front, up)) * dt * speed;
	}

	attitude.x += mouseDy;
	attitude.y += mouseDx;
	attitude.x = attitude.x > 89.f ? 89.f : attitude.x;
	attitude.x = attitude.x < -89.f ? -89.f : attitude.x;

	front = glm::normalize(glm::vec3(
		cos(glm::radians(attitude.y)) * cos(glm::radians(attitude.x)),
		sin(glm::radians(attitude.x)),
		sin(glm::radians(attitude.y)) * cos(glm::radians(attitude.x))
	));
}

void CameraFP::activateView()
{
	view = glm::lookAt(pos, pos + front, up);
}

void CameraFP::setView(glm::mat4 view)
{
	this->view = view;
}

glm::mat4 CameraFP::getView() const
{
	return view;
}

glm::vec3 CameraFP::getPosition() const
{
	return pos;
}
