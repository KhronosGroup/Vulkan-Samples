/* Copyright (c) 2024-2025, Holochip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MapView.h"

MapView::MapView() : mapSize({153.0f, 221.0f})
{

}

MapView::~MapView() = default;

std::vector<VkWriteDescriptorSet> MapView::LoadAssets(ApiVulkanSample *, const VkDescriptorSetAllocateInfo &, VkQueue) {
  return {};
}

void MapView::DrawSidebar() {
  const ImVec4 sidebarColor = ImVec4(
      0x41 / 255.0f,
      0x40 / 255.0f,
      0x42 / 255.0f,
      1.0f
  );

  const ImVec4 buttonColor = ImVec4(
      0x00 / 255.0f,
      0xF1 / 255.0f,
      0xC6 / 255.0f,
      1.0f
  );

  const ImVec4 buttonActiveColor = ImVec4(
      0x00 / 255.0f,
      0x94 / 255.0f,
      0x81 / 255.0f,
      1.0f
  );

  const ImVec4 blackColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

  float oscWindowMainPadding = 20.0f;
  float sidebarExpandedWidth = 240.0f;
  float sidebarButtonWidth = sidebarExpandedWidth - (oscWindowMainPadding * 2);
  float sidebarButtonHeight = 52.0f;
  float buttonSpacing = 10.0f;
  
  // Get available height from the IO display size directly
  ImGuiIO& io = ImGui::GetIO();
  float sidebarHeight = io.DisplaySize.y - (oscWindowMainPadding * 2);

  ImGui::SetCursorPosY(oscWindowMainPadding);
  ImGui::SetCursorPosX(oscWindowMainPadding);

  ImGui::PushStyleColor(ImGuiCol_ChildBg, sidebarColor);
  
  // Create child window - use border=true to help with debugging visibility
  ImGui::BeginChild("sidebar", ImVec2(sidebarExpandedWidth, sidebarHeight), false);

  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
  ImGui::PushStyleColor(ImGuiCol_Text, blackColor);

  // Add initial padding using Dummy
  ImGui::Dummy(ImVec2(0.0f, oscWindowMainPadding - ImGui::GetStyle().ItemSpacing.y));

  // OCTOMAP BUTTON
  ImGui::SetCursorPosX(oscWindowMainPadding);
  
  ImVec4 btnColor1 = (currentState == ViewState::Octomap) ? buttonActiveColor : buttonColor;
  ImGui::PushStyleColor(ImGuiCol_Button, btnColor1);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnColor1);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnColor1);

  if (ImGui::Button("OCTOMAP##btn_octomap", ImVec2(sidebarButtonWidth, sidebarButtonHeight))) {
    if (currentState != ViewState::Octomap) {
      currentState = ViewState::Octomap;
      stateChanged = true;
    }
  }
  ImGui::PopStyleColor(3);

  // Add spacing between buttons
  ImGui::Dummy(ImVec2(0.0f, buttonSpacing - ImGui::GetStyle().ItemSpacing.y));

  // GLTF REGULAR BUTTON
  ImGui::SetCursorPosX(oscWindowMainPadding);

  ImVec4 btnColor2 = (currentState == ViewState::GLTFRegular) ? buttonActiveColor : buttonColor;
  ImGui::PushStyleColor(ImGuiCol_Button, btnColor2);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnColor2);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnColor2);

  if (ImGui::Button("GLTF MAP##btn_gltf", ImVec2(sidebarButtonWidth, sidebarButtonHeight))) {
    if (currentState != ViewState::GLTFRegular) {
      currentState = ViewState::GLTFRegular;
      stateChanged = true;
    }
  }
  ImGui::PopStyleColor(3);

  // Add spacing between buttons
  ImGui::Dummy(ImVec2(0.0f, buttonSpacing - ImGui::GetStyle().ItemSpacing.y));

  // GAUSSIAN SPLATS BUTTON
  ImGui::SetCursorPosX(oscWindowMainPadding);

  ImVec4 btnColor3 = (currentState == ViewState::GLTFSplats) ? buttonActiveColor : buttonColor;
  ImGui::PushStyleColor(ImGuiCol_Button, btnColor3);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnColor3);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnColor3);

  if (ImGui::Button("SPLATS##btn_splats", ImVec2(sidebarButtonWidth, sidebarButtonHeight))) {
    if (currentState != ViewState::GLTFSplats) {
      currentState = ViewState::GLTFSplats;
      stateChanged = true;
    }
  }
  ImGui::PopStyleColor(3);

  ImGui::PopStyleColor(); // Text color
  ImGui::PopStyleVar();   // FrameRounding

  ImGui::EndChild();
  ImGui::PopStyleColor(); // ChildBg
}

bool MapView::DrawUI() {
  ImGuiStyle &style = ImGui::GetStyle();
  style.ChildRounding = 0.0f;

  const ImVec4 oscSidebarColor = ImVec4(
      0x41 / 255.0f,
      0x40 / 255.0f,
      0x42 / 255.0f,
      1.0f
  );

  float oscWindowMainPadding = 20.0f;

  style.WindowRounding = 12.0f;
  style.ChildRounding = 12.0f;
  style.FrameRounding = 12.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  float sidebarExpandedWidth = 240.0f;
  float displaySpaceY = oscWindowMainPadding;
  float displaySpaceX = (oscWindowMainPadding * 2) + sidebarExpandedWidth;
  float displaySpaceHeight = ImGui::GetContentRegionAvail().y - (oscWindowMainPadding * 2);
  float displaySpaceWidth = ImGui::GetContentRegionAvail().x - (oscWindowMainPadding * 3) - sidebarExpandedWidth;

  // Draw the sidebar with buttons
  DrawSidebar();

 	// Draw the main display area
 	ImGui::SetCursorPosY(displaySpaceY);
 	ImGui::SetCursorPosX(displaySpaceX);
 	// Semi-transparent panel background so the UI is visible without fully hiding the 3D map.
 	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0x41 / 255.0f, 0x40 / 255.0f, 0x42 / 255.0f, 0.35f));
 	ImGui::BeginChild("mapDisplay", ImVec2(displaySpaceWidth, displaySpaceHeight), false);
 	ImGui::PopStyleColor();
 	ImGui::EndChild();

  mapSize = {displaySpaceWidth, displaySpaceHeight};
  mapPos = {displaySpaceX, displaySpaceY};

  ImGui::PopStyleVar();
  ImGui::PopStyleVar();

  return stateChanged;
}
