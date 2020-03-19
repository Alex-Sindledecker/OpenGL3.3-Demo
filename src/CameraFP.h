#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <SFML/Window/Event.hpp>

//
class CameraFP
{
public:
	CameraFP(glm::vec3 pos, float walkSpeed);
	~CameraFP();

	void setBounds(glm::vec2 xBounds, glm::vec2 zBounds);
	void setPosition(glm::vec3 pos);
	void setPosition(float x, float y, float z);
	void move(glm::vec3 displacement);
	void move(float dx, float dy, float dz);
	void inputProc(sf::Event& event, float dt, float mouseDx, float mouseDy);
	void activateView();
	void setView(glm::mat4 view);
	glm::mat4 getView() const;
	glm::vec3 getPosition() const;

private:
	float walkSpeed, sprintSpeed;
	sf::Keyboard::Key sprintButton;
	glm::mat4 view;
	glm::vec3 pos, front, up, attitude;
	glm::vec2 xBounds, zBounds;
};

