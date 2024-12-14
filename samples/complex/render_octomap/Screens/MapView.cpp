//
// Created by swinston on 5/22/24.
//

#include "MapView.h"

MapView::MapView() : mapSize({153.0f, 221.0f})
{

}

MapView::~MapView() = default;

std::vector<VkWriteDescriptorSet> MapView::LoadAssets(ApiVulkanSample *, const VkDescriptorSetAllocateInfo &, VkQueue) {
  return {};
}

bool MapView::DrawUI() {
  ImGuiStyle &style = ImGui::GetStyle();
  style.ChildRounding = 0.0f;

  const ImVec4 oscSidebarColor = ImVec4(
      0x41 / 255.0f, // Red component
      0x40 / 255.0f, // Green component
      0x42 / 255.0f, // Blue component
      1.0f           // Alpha component, fully opaque
  );

  float oscWindowHeight = 1080;
  float oscWindowWidth = 1920;
  float oscWindowMainPadding = 20.0f;

  ImGui::SetNextWindowSize(ImVec2(oscWindowWidth, oscWindowHeight));


  style.WindowRounding = 12.0f;
  style.ChildRounding = 12.0f;
  style.FrameRounding = 12.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(100, 100));

  float sidebarExpandedWidth = (240.0f);
  float displaySpaceY = oscWindowMainPadding;
  float displaySpaceX = ((oscWindowMainPadding * 2) + sidebarExpandedWidth);
  float displaySpaceHeight = (ImGui::GetContentRegionAvail().y - (oscWindowMainPadding * 2));
  float displaySpaceWidth = (ImGui::GetContentRegionAvail().x - (oscWindowMainPadding * 3) - sidebarExpandedWidth);

  ImGui::SetCursorPosY(displaySpaceY);
  ImGui::SetCursorPosX(displaySpaceX);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(oscSidebarColor));
  ImGui::BeginChild("liveMaps", ImVec2(displaySpaceWidth, displaySpaceHeight), false);
  ImGui::PopStyleColor();
  ImGui::EndChild();
  ImGui::SetCursorPosY(0);
  ImGui::SetCursorPosX(0);

  mapSize = {displaySpaceWidth, displaySpaceHeight};
  mapPos = {displaySpaceX, displaySpaceY};

  ImGui::SetCursorPosY(0);
  ImGui::SetCursorPosX(0);
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();

  return false;
}
